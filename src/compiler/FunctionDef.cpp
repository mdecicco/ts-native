#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/Scope.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/bind/ExecutionContext.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

using namespace utils;
using namespace tsn::ffi;

namespace tsn {
    namespace compiler {
        FunctionDef::FunctionDef() {
            m_comp = nullptr;
            m_retTp = nullptr;
            m_retTpSet = false;
            m_thisTp = nullptr;
            m_output = nullptr;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_nextLabelId = 1;
            m_cctxArg = nullptr;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_capsArg = nullptr;
            m_fptrArg = nullptr;
            m_retpArg = nullptr;
            m_poison = nullptr;
            m_ownCaptureData = nullptr;
            m_captureDataInsertAddr = 0;
            m_captureDataOffset = sizeof(u32);
        }

        FunctionDef::FunctionDef(Compiler* c, const utils::String& name, DataType* methodOf, ParseNode* n) {
            m_node = n;
            m_scopeRef = nullptr;
            m_comp = c;
            m_src = c->getCurrentSrc();
            m_name = name;
            m_retTp = c->getContext()->getTypes()->getType<void>();
            m_retTpSet = false;
            m_thisTp = methodOf;
            m_output = nullptr;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_nextLabelId = 1;
            m_cctxArg = nullptr;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_capsArg = nullptr;
            m_fptrArg = nullptr;
            m_retpArg = nullptr;
            m_poison = &val("@poison", c->getContext()->getTypes()->getType<poison_t>());
            m_ownCaptureData = nullptr;
            m_captureDataInsertAddr = 0;
            m_captureDataOffset = sizeof(u32);
            
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();
            m_argInfo.push({ arg_type::context_ptr, ptrTp });
        }
        
        FunctionDef::FunctionDef(Compiler* c, Function* func, ParseNode* n) {
            m_node = n;
            m_scopeRef = nullptr;
            m_comp = c;
            m_src = c->getCurrentSrc();
            m_name = func->getName();
            m_thisTp = nullptr;
            m_retTp = c->getContext()->getTypes()->getType<void>();
            m_retTpSet = false;
            m_output = func;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_nextLabelId = 1;
            m_cctxArg = nullptr;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_capsArg = nullptr;
            m_fptrArg = nullptr;
            m_retpArg = nullptr;
            m_poison = &val("@poison", c->getContext()->getTypes()->getType<poison_t>());
            m_ownCaptureData = nullptr;
            m_captureDataInsertAddr = 0;
            m_captureDataOffset = sizeof(u32);

            FunctionType* sig = func->getSignature();
            if (sig) {
                m_retTp = sig->getReturnType();
                m_retTpSet = true;
                const auto& args = sig->getArguments();
                for (u8 a = 0;a < args.size();a++) {
                    m_argInfo.push(args[a]);
                }
            }
        }

        FunctionDef::~FunctionDef() {
            if (m_cctxArg) delete m_cctxArg;
            m_cctxArg = nullptr;

            if (m_fptrArg) delete m_fptrArg;
            m_fptrArg = nullptr;

            if (m_retpArg) delete m_retpArg;
            m_retpArg = nullptr;

            if (m_ectxArg) delete m_ectxArg;
            m_ectxArg = nullptr;

            if (m_capsArg) delete m_capsArg;
            m_capsArg = nullptr;

            if (m_thisArg) delete m_thisArg;
            m_thisArg = nullptr;

            if (m_ownCaptureData) delete m_ownCaptureData;
            m_ownCaptureData = nullptr;

            for (u32 i = 0;i < m_args.size();i++) delete m_args[i];
            m_args.clear();

            for (u32 i = 0;i < m_captures.size();i++) delete m_captures[i].decl;
            m_captures.clear();
        }

        InstructionRef FunctionDef::add(ir_instruction i) {
            m_instructions.push(Instruction(i, m_comp->getCurrentSrc()));
            return InstructionRef(this, m_instructions.size() - 1);
        }

        label_id FunctionDef::label() {
            label_id l = m_nextLabelId++;
            add(ir_label).label(l);
            return l;
        }

        Compiler* FunctionDef::getCompiler() const {
            return m_comp;
        }

        Context* FunctionDef::getContext() const {
            return m_comp->getContext();
        }
        
        const utils::String& FunctionDef::getName() const {
            return m_name;
        }

        void FunctionDef::setReturnType(DataType* tp) {
            m_retTp = tp;
            m_argInfo[1].dataType = tp;
            m_retTpSet = true;
            if (m_retpArg) m_retpArg->setType(tp);
        }

        void FunctionDef::setThisType(ffi::DataType* tp) {
            m_thisTp = tp;
        }

        DataType* FunctionDef::getReturnType() const {
            return m_retTp;
        }

        bool FunctionDef::isReturnTypeExplicit() const {
            return m_retTpSet;
        }

        DataType* FunctionDef::getThisType() const {
            return m_thisTp;
        }

        const SourceLocation& FunctionDef::getSource() const {
            return m_src;
        }

        u32 FunctionDef::getArgCount() const {
            return m_argInfo.size() - 1;
        }

        void FunctionDef::addArg(const utils::String& name, DataType* tp) {
            m_argNames.push(name);
            m_argInfo.push({
                tp->getInfo().is_primitive ? arg_type::value : arg_type::pointer,
                tp
            });

            Value& s = val(name, tp);
            s.m_flags.is_argument = 1;
            s.m_flags.is_function = tp->getInfo().is_function;
            s.m_regId = 0;
            s.m_imm.u = m_argInfo.size() - 1;
            m_args.push(new Value(s));
        }
        
        void FunctionDef::addDeferredArg(const utils::String& name) {
            u32 idx = m_args.size() + 1;
            if (idx >= m_argInfo.size()) {
                m_comp->error(cm_err_internal, "Attempted to add deferred argument that is out of range");
                return;
            }

            Value& s = val(name, m_argInfo[idx].dataType);
            s.m_flags.is_argument = 1;
            s.m_flags.is_function = m_argInfo[idx].dataType->getInfo().is_function;
            s.m_regId = 0;
            s.m_imm.u = idx;
            m_args.push(new Value(s));
        }
        
        const function_argument& FunctionDef::getArgInfo(u32 argIdx) const {
            return m_argInfo[argIdx + 1];
        }
        
        const utils::Array<ffi::function_argument>& FunctionDef::getArgs() const {
            return m_argInfo;
        }

        Value& FunctionDef::getArg(u32 argIdx) {
            return *m_args[argIdx];
        }

        Value& FunctionDef::getThis() {
            if (!m_thisArg) {
                m_comp->error(cm_err_this_outside_class, "Use of 'this' keyword outside of class scope");
                return *m_poison;
            }

            return *m_thisArg;
        }

        Value& FunctionDef::getCCtx() {
            return *m_cctxArg;
        }

        Value& FunctionDef::getECtx() {
            return *m_ectxArg;
        }
        
        Value& FunctionDef::getCaptures() {
            return *m_capsArg;
        }
        
        Value& FunctionDef::getFPtr() {
            return *m_fptrArg;
        }

        Value& FunctionDef::getRetPtr() {
            return *m_retpArg;
        }

        Value& FunctionDef::getPoison() {
            return *m_poison;
        }

        Value& FunctionDef::getOwnCaptureData() {
            return *m_ownCaptureData;
        }

        Value FunctionDef::getNull() {
            Value v = imm<u64>(0);
            v.m_type = m_comp->getContext()->getTypes()->getType<null_t>();
            return v;
        }

        u32 FunctionDef::capture(const Value& val) {
            if (val.getName().size() == 0) {
                throw "Attempt to capture anonymous value";
            }

            for (u32 i = 0;i < m_captures.size();i++) {
                if (m_captures[i].decl->getName() == val.getName()) {
                    return m_captures[i].offset;
                }
            }

            m_captureDataOffset += sizeof(type_id);
            u32 offset = m_captureDataOffset;
            m_captureDataOffset += val.getType()->getInfo().size;

            m_captures.push({
                offset,
                m_instructions.size(),
                new Value(val)
            });

            return offset;
        }

        alloc_id FunctionDef::reserveStackId() {
            return m_nextAllocId++;
        }

        void FunctionDef::setStackId(Value& v, alloc_id id) {
            v.m_allocId = id;
            v.m_regId = 0;
        }

        Value& FunctionDef::promote(const Value& v, const utils::String& name, Scope* scope) {
            if (v.getName().size() > 0) {
                return *m_poison;
            }

            if (v.isImm()) {
                Value& out = val(name, v.getType());
                out = v;
                return out;
            }

            if (!scope) scope = &m_comp->scope().get();

            Value* out = new Value(v);
            out->m_name = name;
            scope->add(name, out);

            m_instructions.each([&v, &name](Instruction& i) {
                for (u8 o = 0;o < i.oCnt;o++) {
                    Value& op = i.operands[o];
                    if (op.isReg()) {
                        if (v.isReg() && op.m_regId == v.m_regId) {
                            op.m_name = name;
                        }
                    } else if (op.isStack()) {
                        if (v.isStack() && op.m_slotId == v.m_slotId) {
                            op.m_name = name;
                        }
                    }
                }
            });

            return *out;
        }

        Value& FunctionDef::val(const utils::String& name, DataType* tp, Scope* scope) {
            if (!scope) scope = &m_comp->scope().get();

            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(this, tp);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            scope->add(name, v);
            return *v;
        }
        
        Value& FunctionDef::val(const utils::String& name, u32 module_data_slot, Scope* scope) {
            return val(name, m_comp->getOutput()->getModule(), module_data_slot, scope);
        }
        
        Value& FunctionDef::val(const utils::String& name, Module* m, u32 module_data_slot, Scope* scope) {
            if (!scope) scope = &m_comp->scope().get();
            
            const module_data& info = m->getDataInfo(module_data_slot);

            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(this, info.type);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            v->m_flags.is_pointer = 1;

            add(ir_module_data).op(*v).op(imm<u32>(m->getId())).op(imm<u32>(module_data_slot));
            scope->add(name, v);
            return *v;
        }

        Value FunctionDef::val(Module* m, u32 module_data_slot) {
            const module_data& info = m->getDataInfo(module_data_slot);

            Value v = Value(this, info.type ? info.type : m_comp->getContext()->getTypes()->getVoidPtr());
            v.m_regId = m_nextRegId++;
            v.m_flags.is_pointer = 1;

            add(ir_module_data).op(v).op(imm<u32>(m->getId())).op(imm<u32>(module_data_slot));
            return v;
        }

        Value FunctionDef::val(DataType* tp) {
            Value v = Value(this, tp);
            v.m_regId = m_nextRegId++;
            return v;
        }

        Value FunctionDef::stack(DataType* tp, bool unscoped, const utils::String& allocComment, const utils::String& ptrComment) {
            Value v = Value(this, tp);

            alloc_id stackId = reserveStackId();
            v.setStackAllocId(stackId);
            add(ir_stack_allocate).op(imm(tp->getInfo().size)).op(imm(stackId)).comment(allocComment);
            if (!unscoped) {
                m_comp->scope().get().addToStack(v);
                Value out = val(tp);
                add(ir_stack_ptr).op(out).op(imm(stackId)).comment(ptrComment);
                out.m_flags.is_pointer = 1;
                out.setStackRef(v);
                return out;
            }

            return v;
        }

        Value FunctionDef::imm(ffi::DataType* tp) {
            return Value(this, tp, true);
        }

        void FunctionDef::onEnter() {
            DataType* voidp = m_comp->getContext()->getTypes()->getType<void*>();
            DataType* ectx = m_comp->getContext()->getTypes()->getType<ExecutionContext>();
            DataType* errt = m_comp->getContext()->getTypes()->getType<poison_t>();

            Value cctxParam = val(voidp);
            cctxParam.m_flags.is_argument = 1;
            cctxParam.m_flags.is_pointer = 1;
            cctxParam.m_regId = 0;
            cctxParam.m_imm.u = 0;
            m_cctxArg = new Value(cctxParam);

            Value& c = val("@ectx", ectx);
            c.m_flags.is_pointer = 1;
            add(ir_load).op(c).op(cctxParam).op(imm<u32>(offsetof(call_context, ectx)));
            m_ectxArg = new Value(c);

            Value& f = val("@fptr", voidp);
            f.m_flags.is_pointer = 1;
            add(ir_load).op(f).op(cctxParam).op(imm<u32>(offsetof(call_context, funcPtr)));
            m_fptrArg = new Value(f);

            Value& r = val("@ret", m_retTpSet ? m_retTp : voidp);
            r.m_flags.is_pointer = 1;
            add(ir_load).op(r).op(cctxParam).op(imm<u32>(offsetof(call_context, retPtr)));
            m_retpArg = new Value(r);

            Value& cp = val("@caps", voidp);
            cp.m_flags.is_pointer = 1;
            add(ir_load).op(cp).op(cctxParam).op(imm<u32>(offsetof(call_context, capturePtr)));
            m_capsArg = new Value(cp);

            if (m_thisTp) {
                Value& t = val("this", m_thisTp);
                t.m_flags.is_pointer = 1;
                add(ir_load).op(t).op(cctxParam).op(imm<u32>(offsetof(call_context, thisPtr)));
                m_thisArg = new Value(t);
            }

            Value& p = val("@poison", errt);

            Value cd = val("@ocaps", voidp);
            cd.m_flags.is_pointer = 1;
            m_ownCaptureData = new Value(cd);
            m_captureDataInsertAddr = m_instructions.size();
            // Value initialization will be handled later,
            // instructions will be inserted at the start of the function
        }

        Function* FunctionDef::onExit() {
            if (m_captures.size() > 0) {
                Array<Instruction> backupInstructions = m_instructions;
                m_instructions.clear(false);

                ffi::Function* allocCaptures = m_comp->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                    return fn && fn->getName() == "$allocCaptures";
                });

                Value count = imm(m_captures.size());
                
                m_comp->m_trustEnable = true;
                Value data = m_comp->generateCall(allocCaptures, { imm(m_captureDataOffset), count });
                add(ir_assign).op(*m_ownCaptureData).op(data);
                m_comp->m_trustEnable = false;

                u32 offset = 0;

                // write capture count
                add(ir_store).op(count).op(*m_ownCaptureData);
                offset += sizeof(u32);

                for (u32 i = 0;i < m_captures.size();i++) {
                    auto& v = m_captures[i];
                    DataType* tp = v.decl->getType();
                    type_id tid = tp->getId();
                    u32 size = tp->getInfo().size;

                    add(ir_noop).comment(utils::String::Format("---- capture[%d] '%s'", i, v.decl->getName().c_str()));

                    // write capture type id
                    add(ir_store).op(imm(tid)).op(*m_ownCaptureData).op(imm(offset));
                    offset += sizeof(type_id);
                    offset += size;
                }

                struct insert_instr {
                    Instruction i;
                    u32 addr;
                };
                Array<insert_instr> inserts; 

                for (u32 i = 0;i < m_captures.size();i++) {
                    auto& v = m_captures[i];
                    if (!v.decl->getType()->getInfo().is_primitive) {
                        // Non-primitive captures live exclusively in the capture data,
                        // effectively dynamically allocated. No load instructions necessary
                        // because all uses of them are already pointers to the capture data
                        continue;
                    }

                    // Store the value in the capture data initially
                    {
                        inserts.push({
                            Instruction(ir_store, m_src),
                            v.firstCapturedAt + 1
                        });
                        Instruction& l = inserts.last().i;
                        l.operands[0].reset(*v.decl);
                        l.operands[1].reset(*m_ownCaptureData);
                        l.operands[2].reset(imm(v.offset));
                        l.oCnt = 3;
                    }

                    // after each assignment to the captured variable we must store the value
                    // in the capture data
                    // before each read from the captured variable we must load the value from
                    // the capture data
                    for (u32 i = v.firstCapturedAt;i < backupInstructions.size();i++) {
                        if (backupInstructions[i].involves(v.decl->getRegId(), true)) {
                            // reads from variable
                            inserts.push({
                                Instruction(ir_load, m_src),
                                i
                            });
                            Instruction& l = inserts.last().i;
                            l.operands[0].reset(*v.decl);
                            l.operands[1].reset(*m_ownCaptureData);
                            l.operands[2].reset(imm(v.offset));
                            l.oCnt = 3;
                        }

                        const Value* assigns = backupInstructions[i].assigns();
                        if (assigns && assigns->getRegId() == v.decl->getRegId()) {
                            // writes to variable
                            inserts.push({
                                Instruction(ir_store, m_src),
                                i + 1
                            });
                            Instruction& l = inserts.last().i;
                            l.operands[0].reset(*v.decl);
                            l.operands[1].reset(*m_ownCaptureData);
                            l.operands[2].reset(imm(v.offset));
                            l.oCnt = 3;
                        }
                    }
                }

                for (u32 i = 0;i < inserts.size();i++) {
                    backupInstructions.insert(inserts[i].addr, inserts[i].i);
                    for (u32 j = i + 1;j < inserts.size();j++) {
                        if (inserts[j].addr >= i) inserts[j].addr++;
                    }
                }

                backupInstructions.insert(m_captureDataInsertAddr, m_instructions);
                m_instructions = backupInstructions;
            }

            DataType* voidTp = m_comp->getContext()->getTypes()->getVoid();
            if (!m_retTp) setReturnType(voidTp);

            if (m_instructions.size() == 0 || m_instructions.last().op != ir_ret) {
                if (m_retTp->isEqualTo(voidTp)) {
                    m_comp->scope().emitReturnInstructions();
                    add(ir_ret);
                } else {
                    m_comp->error(
                        cm_err_function_must_return_a_value,
                        "Function '%s' must return a value of type '%s'",
                        (m_output ? m_output->getDisplayName() : m_name).c_str(),
                        m_retTp->getName().c_str()
                    );
                }
            }

            if (m_output) return m_output;

            utils::Array<ffi::DataType*> argTps;
            for (u32 i = 0;i < m_argInfo.size();i++) {
                if (m_argInfo[i].isImplicit()) continue;
                argTps.push(m_argInfo[i].dataType);
            }

            m_output = new Function(
                m_name,
                m_comp->getOutput()->getModule()->getName() + "::",
                m_comp->getOrCreateFunctionType(m_thisTp, m_retTp, argTps),
                private_access,
                nullptr,
                nullptr,
                m_comp->getOutput()->getModule(),
                m_thisTp
            );

            if (m_scopeRef) {
                m_scopeRef->setType(m_output->getSignature());
            }
            
            m_comp->getOutput()->resolveFunctionDef(this, m_output);

            return m_output;
        }
        
        ffi::Function* FunctionDef::getOutput() {
            return m_output;
        }
        
        const utils::Array<Instruction>& FunctionDef::getCode() const {
            return m_instructions;
        }
    
        utils::Array<Instruction>& FunctionDef::getCode() {
            return m_instructions;
        }
        
        ParseNode* FunctionDef::getNode() {
            return m_node;
        }
    };
};
