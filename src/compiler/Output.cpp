#include <tsn/compiler/Output.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/common/Context.h>
#include <tsn/common/Function.h>
#include <tsn/common/FunctionRegistry.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/Module.h>
#include <tsn/utils/SourceMap.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/utils/ModuleSource.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        Output::Output() {
            m_mod = nullptr;
        }

        Output::Output(OutputBuilder* in) {
            m_mod = in->getModule();

            const auto& funcs = in->getFuncs();
            for (u32 i = 0;i < funcs.size();i++) {
                if (!funcs[i]->getOutput()) continue;
                if (funcs[i]->getOutput()->isTemplate()) continue;
                
                const auto& code = funcs[i]->getCode();

                output::function f;
                f.func = funcs[i]->getOutput();
                f.icount = code.size();
                f.map = new SourceMap(funcs[i]->getSource().getSource()->getModificationTime());
                f.code = new output::instruction[code.size()];
                
                for (u32 c = 0;c < code.size();c++) {
                    auto& inst = f.code[c];
                    const auto& info = instruction_info(code[c].op);
                    inst.op = code[c].op;
                    
                    for (u8 o = 0;o < code[c].oCnt;o++) {
                        auto& v = inst.operands[o];
                        const auto& s = code[c].operands[o];
                        v.data_type = s.getType();
                        v.is_imm = s.isImm();

                        if (v.is_imm) {
                            if (info.operands[o] == ot_fun) v.value.imm_u = s.getImm<FunctionDef*>()->getOutput()->getId();
                            else v.value.imm_u = s.getImm<u64>();
                        }
                        else if (s.isReg()) v.value.reg_id = s.getRegId();
                        else if (s.isStack()) v.value.alloc_id = s.getStackAllocId();
                    }

                    const SourceLocation& src = code[c].src;
                    f.map->add(src.getLine(), src.getCol(), src.getEndLocation().getOffset() - src.getOffset());
                }

                m_funcs.push(f);
            }
        }

        Output::~Output() {
            m_funcs.each([](output::function* f) {
                delete f->map;
                delete [] f->code;
            });
        }

        Module* Output::getModule() {
            return m_mod;
        }

        bool Output::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!out->write(m_mod->getId())) return false;
            if (!out->write(m_mod->getName())) return false;
            if (!out->write(m_mod->getPath())) return false;

            // todo: Serialize everything about the module...

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
                        if (!out->write(inst.operands[o].data_type->getId())) return false;
                        if (!out->write(inst.operands[o].is_imm)) return false;
                        if (!out->write(inst.operands[o].value)) return false;
                    }
                }

                if (!f.map->serialize(out, ctx, extra)) return false;
            }

            return true;
        }

        bool Output::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            ffi::FunctionRegistry* freg = ctx->getFunctions();
            ffi::DataTypeRegistry* treg = ctx->getTypes();

            u32 mid;
            if (!in->read(mid)) return false;
            utils::String name = in->readStr();
            if (name.size() == 0) return false;
            utils::String path = in->readStr();
            if (path.size() == 0) return false;
            
            m_mod = ctx->createModule(name, path);
            if (m_mod->getId() != mid) return false;

            // todo: Deserialize everything about the module...

            u32 fcount;
            if (!in->read(fcount)) return false;
            for (u32 i = 0;i < fcount;i++) {
                output::function f;
                u32 fid;
                if (!in->read(fid)) return false;
                f.func = freg->getFunction(fid);
                if (!f.func) return false;

                if (!in->read(f.icount)) return false;
                f.code = new output::instruction[f.icount];

                for (u32 c = 0;c < f.icount;c++) {
                    auto& inst = f.code[c];
                    inst.operands[0].data_type = nullptr;
                    inst.operands[0].is_imm = false;
                    inst.operands[0].value.imm_u = 0;

                    const auto& info = instruction_info(inst.op);
                    if (!in->read(inst.op)) {
                        delete [] f.code;
                        return false;
                    }

                    for (u8 o = 0;o < info.operand_count;o++) {
                        type_id tid;
                        if (!in->read(tid)) {
                            delete [] f.code;
                            return false;
                        }

                        inst.operands[o].data_type = treg->getType(tid);
                        if (!inst.operands[o].data_type) {
                            delete [] f.code;
                            return false;
                        }

                        if (!in->read(inst.operands[o].is_imm)) {
                            delete [] f.code;
                            return false;
                        }

                        if (!in->read(inst.operands[o].value)) {
                            delete [] f.code;
                            return false;
                        }
                    }
                }
            
                f.map = new SourceMap();
                if (!f.map->deserialize(in, ctx, extra)) {
                    delete [] f.code;
                    delete f.map;
                    return false;
                }

                m_funcs.push(f);
            }

            return true;
        }
    };
};