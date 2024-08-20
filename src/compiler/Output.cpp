#include <tsn/compiler/Output.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/TemplateContext.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/io/Workspace.h>
#include <tsn/utils/SourceMap.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/utils/ModuleSource.h>
#include <tsn/pipeline/Pipeline.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        using namespace output;

        struct host_specialization {
            type_id id;
            type_id base_type;
            utils::Array<type_id> args;
        };
        ffi::DataType* findType(
            robin_hood::unordered_map<type_id, ffi::DataType*>& typeMap,
            const robin_hood::unordered_map<type_id, host_specialization>& hostSpecializations,
            ffi::DataTypeRegistry* treg,
            type_id tid
        ) {
            auto it = typeMap.find(tid);
            if (it != typeMap.end()) return it->second;

            ffi::DataType* tp = treg->getType(tid);
            if (tp) return tp;

            auto h = hostSpecializations.find(tid);
            if (h == hostSpecializations.end()) return nullptr;

            auto s = h->second;

            // it's time...
            ffi::DataType* base = findType(typeMap, hostSpecializations, treg, s.base_type);
            if (!base) return nullptr;

            utils::Array<ffi::DataType*> args;
            for (u32 i = 0;i < s.args.size();i++) {
                ffi::DataType* a = findType(typeMap, hostSpecializations, treg, s.args[i]);
                if (!a) return nullptr;

                args.push(a);
            }

            ffi::DataType* out = treg->getContext()->getPipeline()->specializeTemplate((ffi::TemplateType*)base, args);
            if (!out) return nullptr;

            typeMap[tid] = out;
            return out;
        }

        Output::Output(const script_metadata* meta, ModuleSource* src) {
            m_mod = nullptr;
            m_src = src;
            m_meta = meta;
            m_didDeserializeDeps = false;
        }

        Output::Output(OutputBuilder* in) {
            m_mod = in->getModule();
            m_src = m_mod->getSource();
            m_meta = m_mod->getInfo();
            m_dependencies = in->getDependencies();
            m_specializedHostTypes = in->getSpecializedHostTypes();

            const auto& funcs = in->getFuncs();
            for (u32 i = 0;i < funcs.size();i++) {
                if (!funcs[i]->getOutput()) continue;
                if (funcs[i]->getOutput()->getFlags().is_template) continue;

                CodeHolder* ch = new CodeHolder(funcs[i]->getCode());
                ch->owner = funcs[i]->getOutput();
                m_output.push(ch);
            }
        }

        Output::~Output() {
            m_funcs.each([](output::function* f) {
                delete f->map;
                delete [] f->code;
            });

            m_output.each([](CodeHolder* ch) {
                delete ch;
            });
        }

        void Output::processInput() {
            ffi::DataType* offsetTp = m_mod->getContext()->getTypes()->getUInt32();

            for (u32 i = 0;i < m_output.size();i++) {
                const auto& code = m_output[i]->code;

                output::function f;
                f.func = m_output[i]->owner;
                f.icount = code.size();
                f.map = new SourceMap(m_mod->getInfo()->modified_on);
                f.map->setSource(m_src);
                f.code = new output::instruction[code.size()];
                
                for (u32 c = 0;c < code.size();c++) {
                    auto& inst = f.code[c];
                    const auto& info = instruction_info(code[c].op);
                    inst.op = code[c].op;
                    
                    for (u8 o = 0;o < info.operand_count;o++) {
                        auto& v = inst.operands[o];
                        const auto& s = code[c].operands[o];

                        if (o == 2 && (inst.op == ir_load || inst.op == ir_store) && !s.isValid()) {
                            // We don't store the operand count to conserve space so optional parameters
                            // which are undefined must be converted into ineffectual defaults
                            v.data_type = offsetTp;
                            v.flags.is_reg = 0;
                            v.flags.is_stack = 0;
                            v.flags.is_func = 0;
                            v.flags.is_imm = 1;
                            v.flags.is_arg = 0;
                            v.flags.is_pointer = 0;
                            v.value.imm_u = 0;
                        } else {
                            v.data_type = s.getType();
                            v.flags.is_reg = s.isReg() ? 1 : 0;
                            v.flags.is_stack = s.isStack() ? 1 : 0;
                            v.flags.is_func = s.isFunction() ? 1 : 0;
                            v.flags.is_imm = s.isImm() ? 1 : 0;
                            v.flags.is_arg = s.isArg() ? 1 : 0;
                            v.flags.is_pointer = s.getFlags().is_pointer;
                        }

                        if (s.isReg()) v.value.reg_id = s.getRegId();
                        else if (s.isArg()) v.value.arg_idx = s.getImm<u32>();
                        else if (s.isStack()) v.value.alloc_id = s.getStackAllocId();
                        else if (v.flags.is_imm) {
                            if (info.operands[o] == ot_fun) {
                                if (s.isFunctionID()) v.value.imm_u = s.getImm<function_id>();
                                else v.value.imm_u = s.getImm<FunctionDef*>()->getOutput()->getId();
                            } else v.value.imm_u = s.getImm<u64>();
                        }
                    }

                    const SourceLocation& src = code[c].src;
                    f.map->add(src.getLine(), src.getCol(), src.getEndLocation().getOffset() - src.getOffset());
                }

                m_funcs.push(f);
            }
        }

        Module* Output::getModule() {
            return m_mod;
        }
        
        const utils::Array<compiler::CodeHolder*>& Output::getCode() const {
            return m_output;
        }
        
        const utils::Array<Module*>& Output::getDependencies() const {
            return m_dependencies;
        }
        
        const utils::Array<u64>& Output::getDependencyVersions() const {
            return m_dependencyVersions;
        }

        bool Output::serialize(utils::Buffer* out, Context* ctx) const {
            if (!out->write(m_dependencies.size())) return false;
            for (u32 i = 0;i < m_dependencies.size();i++) {
                if (!out->write(m_dependencies[i]->getPath())) return false;

                u64 version = m_dependencies[i]->getInfo()->modified_on;
                if (!out->write(version)) return false;
            }

            if (!out->write(m_specializedHostTypes.size())) return false;
            for (u32 i = 0;i < m_specializedHostTypes.size();i++) {
                ffi::DataType* tp = m_specializedHostTypes[i];
                if (!out->write(tp->getId())) return false;
                if (!out->write(tp->getTemplateBase()->getId())) return false;
                
                auto targs = tp->getTemplateArguments();
                if (!out->write(targs.size())) return false;
                for (u32 a = 0;a < targs.size();a++) {
                    if (!out->write(targs[a]->getId())) return false;
                }
            }

            if (!out->write(m_mod->getId())) return false;
            if (!out->write(m_mod->getName())) return false;
            if (!out->write(m_mod->getPath())) return false;

            // Minus one because first function in IFunctionHolder is always null
            const auto funcs = m_mod->allFunctions();
            if (!out->write(funcs.size() - 1)) return false;
            for (u32 i = 1;i < funcs.size();i++) {
                if (!funcs[i]->serialize(out, ctx)) return false;
            }

            const auto& types = m_mod->allTypes();
            if (!out->write(types.size())) return false;
            for (u32 i = 0;i < types.size();i++) {
                if (!types[i]->serialize(out, ctx)) return false;
            }

            const auto& data = m_mod->getData();
            if (!out->write(data.size())) return false;
            for (u32 i = 0;i < data.size();i++) {
                if (!out->write(data[i].type ? data[i].type->getId() : 0)) return false;
                if (!out->write(data[i].size)) return false;
                if (!out->write(data[i].access)) return false;
                if (!out->write(data[i].name)) return false;
                if (!out->write(data[i].ptr, data[i].size)) return false;
            }

            if (!out->write(m_funcs.size())) return false;

            for (u32 i = 0;i < m_funcs.size();i++) {
                const auto& f = m_funcs[i];

                if (!out->write(f.func->getId())) return false;
                if (!out->write(f.icount)) return false;

                for (u32 c = 0;c < f.icount;c++) {
                    const auto& inst = f.code[c];
                    const auto& info = instruction_info(inst.op);
                    if (!out->write(inst.op)) return false;

                    for (u8 o = 0;o < info.operand_count;o++) {
                        if (!out->write(inst.operands[o].flags)) return false;
                        if (!out->write(inst.operands[o].data_type->getId())) return false;
                        if (!out->write(inst.operands[o].value)) return false;
                    }
                }

                if (!f.map->serialize(out, ctx)) return false;
            }

            return true;
        }

        bool Output::deserializeDependencies(utils::Buffer* in, Context* ctx) {
            u32 count;
            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                utils::String path = in->readStr();
                if (path.size() == 0) return false;

                u64 version;
                if (!in->read(version)) return false;

                Module* mod = ctx->getModule(path);
                if (!mod) return false;

                m_dependencies.push(mod);
                m_dependencyVersions.push(version);
            }

            m_didDeserializeDeps = true;

            return true;
        }

        bool Output::deserialize(utils::Buffer* in, Context* ctx) {
            utils::Array<proto_function> proto_funcs;
            utils::Array<proto_type> proto_types;
            utils::Array<TemplateContext*> tcontexts;
            ffi::FunctionRegistry* freg = ctx->getFunctions();
            ffi::DataTypeRegistry* treg = ctx->getTypes();
            u32 count;

            auto onFailure = [this, ctx, &tcontexts]() {
                tcontexts.each([](TemplateContext* t) { delete t; });
                if (m_mod) ctx->destroyModule(m_mod, true);
                return false;
            };

            if (!m_didDeserializeDeps) {
                if (!in->read(count)) return onFailure();
                for (u32 i = 0;i < count;i++) {
                    utils::String path = in->readStr();
                    if (path.size() == 0) return onFailure();

                    u64 version;
                    if (!in->read(version)) return onFailure();

                    Module* mod = ctx->getModule(path);
                    if (!mod) return onFailure();

                    m_dependencies.push(mod);
                    m_dependencyVersions.push(version);
                }
            }

            robin_hood::unordered_map<u32, host_specialization> hostSpecializations;

            if (!in->read(count)) return onFailure();
            hostSpecializations.reserve(count);

            for (u32 i = 0;i < count;i++) {
                type_id selfId, baseId;
                if (!in->read(selfId)) return onFailure();
                if (!in->read(baseId)) return onFailure();

                host_specialization& s = hostSpecializations[selfId] = {
                    selfId, baseId, {}
                };
                
                u32 argc;
                if (!in->read(argc)) return onFailure();
                for (u32 a = 0;a < argc;a++) {
                    type_id argId;
                    if (!in->read(argId)) return false;

                    s.args.push(argId);
                }
            }

            u32 mid;
            if (!in->read(mid)) return onFailure();
            utils::String name = in->readStr();
            if (name.size() == 0) return onFailure();
            utils::String path = in->readStr();
            if (path.size() == 0) return onFailure();
            
            m_mod = ctx->createModule(name, path, m_meta);
            if (!m_mod) return onFailure();
            if (m_mod->getId() != mid) return onFailure();
            m_mod->setSrc(m_src);

            // read functions
            if (!in->read(count)) return onFailure();
            for (u32 i = 0;i < count;i++) {
                function_id id;
                if (!in->read(id)) return onFailure();
                proto_funcs.push(proto_function());
                proto_function& pf = proto_funcs.last();
                pf.id = id;

                pf.name = in->readStr();
                if (pf.name.size() == 0) return onFailure();
                pf.displayName = in->readStr();
                pf.fullyQualifiedName = in->readStr();

                if (!in->read(pf.access)) return onFailure();
                if (!in->read(pf.signatureTypeId)) return onFailure();
                if (!in->read(pf.ownerId)) return onFailure();
                if (!in->read(pf.flags)) return onFailure();
                if (!pf.src.deserialize(in, ctx)) return onFailure();
                
                if (pf.flags.is_method) {
                    if (!in->read(pf.baseOffset)) return onFailure();
                }

                if (pf.flags.is_template) {
                    pf.tctx = new TemplateContext();
                    if (!pf.tctx->deserialize(in, ctx)) {
                        delete pf.tctx;
                        pf.tctx = nullptr;
                        return onFailure();
                    }

                    tcontexts.push(pf.tctx);
                    pf.tctx->getAST()->rehydrateSourceRefs(pf.tctx->getOrigin()->getSource());
                }
            }

            // read types
            if (!in->read(count)) return onFailure();
            for (u32 i = 0;i < count;i++) {
                type_id id;
                if (!in->read(id)) return onFailure();
                proto_types.push(proto_type());
                proto_type& pt = proto_types.last();
                pt.id = id;

                if (!in->read(pt.itype)) return onFailure();

                u32 sourceModuleId;
                if (!in->read(sourceModuleId)) return onFailure();
                pt.sourceModule = ctx->getModule(sourceModuleId);
                if (!pt.sourceModule) return onFailure();

                pt.name = in->readStr();
                if (pt.name.size() == 0) return onFailure();
                pt.fullyQualifiedName = in->readStr();
                if (pt.fullyQualifiedName.size() == 0) return onFailure();
                if (!in->read(pt.info)) return onFailure();
                if (!in->read(pt.access)) return onFailure();
                if (!in->read(pt.destructorId)) return onFailure();
                
                u32 pcount;
                if (!in->read(pcount)) return onFailure();
                for (u32 p = 0;p < pcount;p++) {
                    pt.props.push(proto_type_prop());
                    proto_type_prop& prop = pt.props.last();

                    prop.name = in->readStr();
                    if (prop.name.size() == 0) return onFailure();

                    if (!in->read(prop.access)) return onFailure();
                    if (!in->read(prop.offset)) return onFailure();
                    if (!in->read(prop.typeId)) return onFailure();
                    if (!in->read(prop.flags)) return onFailure();
                    if (!in->read(prop.getterId)) return onFailure();
                    if (!in->read(prop.setterId)) return onFailure();
                }

                u32 bcount;
                if (!in->read(bcount)) return onFailure();
                pt.bases.reserve(bcount);
                for (u32 i = 0;i < bcount;i++) {
                    pt.bases.push(proto_type_base());
                    if (!in->read(pt.bases.last())) return onFailure();
                }

                u32 mcount;
                if (!in->read(mcount)) return onFailure();
                pt.methodIds.reserve(mcount);
                for (u32 i = 0;i < mcount;i++) {
                    function_id mid;
                    if (!in->read(mid)) return onFailure();
                    pt.methodIds.push(mid);
                }

                if (!in->read(pt.templateBaseId)) return onFailure();
                u32 tacount;
                if (!in->read(tacount)) return onFailure();
                for (u32 i = 0;i < tacount;i++) {
                    type_id tid;
                    if (!in->read(tid)) return onFailure();
                    pt.templateArgIds.push(tid);
                }

                switch (pt.itype) {
                    case ffi::dti_plain: break;
                    case ffi::dti_function: {
                        if (!in->read(pt.returnTypeId)) return onFailure();
                        if (!in->read(pt.thisTypeId)) return onFailure();
                        if (!in->read(pt.returnsPointer)) return onFailure();
                        u32 acount;
                        if (!in->read(acount)) return onFailure();
                        for (u32 a = 0;a < acount;a++) {
                            proto_type_arg arg;
                            if (!in->read(arg.argType)) return onFailure();
                            if (!in->read(arg.dataTypeId)) return onFailure();
                            pt.args.push(arg);
                        }
                        break;
                    }
                    case ffi::dti_template: {
                        pt.tctx = new TemplateContext();
                        if (!pt.tctx->deserialize(in, ctx)) {
                            delete pt.tctx;
                            pt.tctx = nullptr;
                            return onFailure();
                        }
                    
                        tcontexts.push(pt.tctx);
                        pt.tctx->getAST()->rehydrateSourceRefs(m_mod->getSource());
                        break;
                    }
                    case ffi::dti_alias: {
                        if (!in->read(pt.aliasTypeId)) return onFailure();
                        break;
                    }
                    case ffi::dti_class: break;
                }
            }

            utils::Array<ffi::Function*> ofuncs(proto_funcs.size());
            utils::Array<ffi::DataType*> otypes(proto_types.size());
            robin_hood::unordered_map<function_id, ffi::Function*> funcMap;
            robin_hood::unordered_map<type_id, ffi::DataType*> typeMap;

            auto getFnById = [&funcMap, freg](function_id fid) {
                auto it = funcMap.find(fid);
                if (it != funcMap.end()) return it->second;

                return freg->getFunction(fid);
            };

            auto getTpById = [&typeMap, treg, &hostSpecializations](type_id tid) {
                return findType(typeMap, hostSpecializations, treg, tid);
            };

            if (!generateTypesAndFunctions(proto_funcs, proto_types, freg, treg, ofuncs, otypes, funcMap, typeMap)) return onFailure();

            // resolve template context data
            for (u32 i = 0;i < tcontexts.size();i++) {
                if (!tcontexts[i]->resolveReferences(freg, treg, funcMap, typeMap)) return onFailure();
            }

            // read data
            if (!in->read(count)) return onFailure();
            for (u32 i = 0;i < count;i++) {
                module_data md;
                type_id tid;
                if (!in->read(tid)) return onFailure();
                md.type = tid == 0 ? nullptr : getTpById(tid);

                if (!in->read(md.size)) return onFailure();
                if (!in->read(md.access)) return onFailure();
                md.name = in->readStr();
                if (md.name.size() == 0) return onFailure();

                if (md.type) m_mod->addData(md.name, md.type, md.access);
                else m_mod->addData(md.name, md.size);
                
                if (!in->read(m_mod->m_data[i].ptr, md.size)) return onFailure();
            }

            if (!in->read(count)) return onFailure();
            for (u32 i = 0;i < count;i++) {
                output::function f;
                u32 fid;
                if (!in->read(fid)) return onFailure();
                f.func = getFnById(fid);
                if (!f.func) return onFailure();

                if (!in->read(f.icount)) return onFailure();
                f.code = new output::instruction[f.icount];

                for (u32 c = 0;c < f.icount;c++) {
                    auto& inst = f.code[c];
                    inst.operands[0].data_type = nullptr;
                    inst.operands[0].value.imm_u = 0;
                    inst.operands[0].flags = { 0 };
                    inst.operands[1].data_type = nullptr;
                    inst.operands[1].value.imm_u = 0;
                    inst.operands[1].flags = { 0 };
                    inst.operands[2].data_type = nullptr;
                    inst.operands[2].value.imm_u = 0;
                    inst.operands[2].flags = { 0 };
                    if (!in->read(inst.op)) {
                        delete [] f.code;
                        return onFailure();
                    }

                    const auto& info = instruction_info(inst.op);
                    for (u8 o = 0;o < info.operand_count;o++) {
                        if (!in->read(inst.operands[o].flags)) {
                            delete [] f.code;
                            return onFailure();
                        }

                        type_id tid;
                        if (!in->read(tid)) {
                            delete [] f.code;
                            return onFailure();
                        }

                        inst.operands[o].data_type = getTpById(tid);
                        if (!inst.operands[o].data_type) {
                            delete [] f.code;
                            return onFailure();
                        }

                        if (!in->read(inst.operands[o].value)) {
                            delete [] f.code;
                            return onFailure();
                        }
                    }
                }
            
                f.map = new SourceMap();
                f.map->setSource(m_src);
                if (!f.map->deserialize(in, ctx)) {
                    delete [] f.code;
                    delete f.map;
                    return onFailure();
                }

                m_funcs.push(f);
            }

            // reconstruct output
            for (u32 f = 0;f < m_funcs.size();f++) {
                const auto& fn = m_funcs[f];

                CodeHolder* ch = new CodeHolder({});
                ch->owner = fn.func;
                ch->code.reserve(fn.icount);

                for (u32 c = 0;c < fn.icount;c++) {
                    auto& ii = fn.code[c];
                    auto& info = instruction_info(ii.op);
                    ch->code.push(Instruction());
                    Instruction& i = ch->code[c];
                    i.op = ii.op;
                    i.oCnt = info.operand_count;

                    for (u8 o = 0;o < info.operand_count;o++) {
                        auto& vi = ii.operands[o];
                        auto& v = i.operands[o];
                        
                        auto& flags = v.getFlags();
                        flags.is_argument = vi.flags.is_arg;
                        flags.is_immediate = vi.flags.is_imm;
                        flags.is_function = vi.flags.is_func;
                        flags.is_pointer = vi.flags.is_pointer;
                        if (flags.is_function) flags.is_function_id = 1;

                        v.setType(vi.data_type);
                        if (vi.flags.is_imm) v.setImm(vi.value.imm_u);
                        else if (vi.flags.is_arg) v.setImm(vi.value.arg_idx);
                        else if (vi.flags.is_reg) v.setRegId(vi.value.reg_id);
                        else if (vi.flags.is_stack) v.setStackAllocId(vi.value.alloc_id);
                    }
                }

                ch->rebuildAll();
                m_output.push(ch);
            }

            for (u32 i = 0;i < ofuncs.size();i++) {
                freg->registerFunction(ofuncs[i]);
                m_mod->addFunction(ofuncs[i]);
            }

            for (u32 i = 0;i < otypes.size();i++) {
                treg->addForeignType(otypes[i]);
                m_mod->addForeignType(otypes[i]);
            }

            return true;
        }

        bool Output::generateTypesAndFunctions(
            utils::Array<proto_function>& funcs,
            utils::Array<proto_type>& types,
            ffi::FunctionRegistry* freg,
            ffi::DataTypeRegistry* treg,
            utils::Array<ffi::Function*>& ofuncs,
            utils::Array<ffi::DataType*>& otypes,
            robin_hood::unordered_map<function_id, ffi::Function*>& funcMap,
            robin_hood::unordered_map<type_id, ffi::DataType*>& typeMap
        ) {
            auto getFnById = [&funcMap, freg](function_id fid) {
                auto it = funcMap.find(fid);
                if (it != funcMap.end()) return it->second;

                return freg->getFunction(fid);
            };

            auto getTpById = [&typeMap, treg](type_id tid) {
                auto it = typeMap.find(tid);
                if (it != typeMap.end()) return it->second;

                return treg->getType(tid);
            };

            // Generate functions without signatures
            for (auto& pf : funcs) {
                ffi::Function* f = nullptr;
                if (pf.flags.is_method) {
                    if (pf.flags.is_template) f = new ffi::TemplateMethod(pf.name, "", pf.access, pf.baseOffset, pf.tctx, pf.ownerId == 0 ? nullptr : getTpById(pf.ownerId));
                    else f = new ffi::Method(pf.name, "", nullptr, pf.access, nullptr, nullptr, pf.baseOffset, m_mod, pf.ownerId == 0 ? nullptr : getTpById(pf.ownerId));
                } else {
                    if (pf.flags.is_template) f = new ffi::TemplateFunction(pf.name, "", pf.access, pf.tctx, pf.ownerId == 0 ? nullptr : getTpById(pf.ownerId));
                    else f = new ffi::Function(pf.name, "", nullptr, pf.access, nullptr, nullptr, m_mod, pf.ownerId == 0 ? nullptr : getTpById(pf.ownerId));
                }

                f->m_fullyQualifiedName = pf.fullyQualifiedName;
                f->m_displayName = pf.displayName;
                f->m_id = pf.id;
                ofuncs.push(f);
                funcMap[f->m_id] = f;
            }

            // Generate types without properties, bases, template info, or derived info
            for (auto& pt : types) {
                ffi::DataType* tp = nullptr;
                switch (pt.itype) {
                    case ffi::dti_plain: { tp = new ffi::DataType(); break; }
                    case ffi::dti_function: { tp = new ffi::FunctionType(); break; }
                    case ffi::dti_template: { tp = new ffi::TemplateType(); break; }
                    case ffi::dti_alias: { tp = new ffi::AliasType(); break; }
                    case ffi::dti_class: { tp = new ffi::ClassType(); break; }
                }

                tp->m_id = pt.id;
                tp->m_sourceModule = pt.sourceModule;
                tp->m_name = pt.name;
                tp->m_fullyQualifiedName = pt.fullyQualifiedName;
                tp->m_info = pt.info;
                tp->m_access = pt.access;
                tp->m_destructor = pt.destructorId == 0 ? nullptr : getFnById(pt.destructorId);

                for (auto mid : pt.methodIds) {
                    tp->m_methods.push(getFnById(mid));
                }

                otypes.push(tp);
                typeMap[tp->m_id] = tp;
            }

            // Add signatures to functions
            for (u32 i = 0;i < funcs.size();i++) {
                proto_function& pf = funcs[i];
                ffi::Function* f = ofuncs[i];

                f->m_signature = pf.signatureTypeId == 0 ? nullptr : (ffi::FunctionType*)getTpById(pf.signatureTypeId);
            }

            // Add properties, bases, template info, derived info to types
            for (u32 i = 0;i < types.size();i++) {
                proto_type& pt = types[i];
                ffi::DataType* tp = otypes[i];

                for (u32 p = 0;p < pt.props.size();p++) {
                    proto_type_prop& pp = pt.props[p];
                    tp->m_properties.push(ffi::type_property());
                    ffi::type_property& prop = tp->m_properties.last();

                    prop.name = pp.name;
                    prop.flags = pp.flags;
                    prop.access = pp.access;
                    prop.offset = pp.offset;
                    prop.type = getTpById(pp.typeId);
                    prop.getter = pp.getterId == 0 ? nullptr : getFnById(pp.getterId);
                    prop.setter = pp.setterId == 0 ? nullptr : getFnById(pp.setterId);
                }

                for (u32 b = 0;b < pt.bases.size();b++) {
                    proto_type_base& pb = pt.bases[b];
                    tp->m_bases.push(ffi::type_base());
                    ffi::type_base& base = tp->m_bases.last();

                    base.access = pb.access;
                    base.offset = pb.offset;
                    base.type = getTpById(pb.typeId);
                }

                if (pt.templateBaseId != 0) {
                    tp->m_templateBase = (ffi::TemplateType*)getTpById(pt.templateBaseId);
                }

                for (u32 a = 0;a < pt.templateArgIds.size();a++) {
                    tp->m_templateArgs.push(getTpById(pt.templateArgIds[a]));
                }

                switch (pt.itype) {
                    case ffi::dti_plain: break;
                    case ffi::dti_function: {
                        ffi::FunctionType* ft = (ffi::FunctionType*)tp;
                        ft->m_returnType = pt.returnTypeId == 0 ? nullptr : getTpById(pt.returnTypeId);
                        ft->m_thisType = pt.thisTypeId == 0 ? nullptr : getTpById(pt.thisTypeId);
                        ft->m_returnsPointer = pt.returnsPointer;
                        
                        for (u32 a = 0;a < pt.args.size();a++) {
                            proto_type_arg& parg = pt.args[a];
                            ft->m_args.push(ffi::function_argument());
                            ffi::function_argument& arg = ft->m_args.last();

                            arg.argType = parg.argType;
                            arg.dataType = getTpById(parg.dataTypeId);
                        }
                        break;
                    }
                    case ffi::dti_template: {
                        ffi::TemplateType* tt = (ffi::TemplateType*)tp;
                        tt->m_data = pt.tctx;
                        break;
                    }
                    case ffi::dti_alias: {
                        ffi::AliasType* at = (ffi::AliasType*)tp;
                        at->m_ref = getTpById(pt.aliasTypeId);
                        break;
                    }
                    case ffi::dti_class: break;
                }
            }

            return true;
        }
    };
};