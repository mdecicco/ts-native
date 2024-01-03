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

            for (u32 i = 0;i < m_args.size();i++) delete m_args[i];
            m_args.clear();
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

        Value FunctionDef::getNull() {
            Value v = imm<u64>(0);
            v.m_type = m_comp->getContext()->getTypes()->getType<null_t>();
            return v;
        }

        alloc_id FunctionDef::reserveStackId() {
            return m_nextAllocId++;
        }

        void FunctionDef::setStackId(Value& v, alloc_id id) {
            v.m_allocId = id;
            v.m_regId = 0;
        }

        Value& FunctionDef::promote(const Value& v, const utils::String& name) {
            if (v.getName().size() > 0) {
                return *m_poison;
            }

            if (v.isImm()) {
                Value& out = val(name, v.getType());
                out = v;
                return out;
            }

            Value* out = new Value(v);
            out->m_name = name;
            m_comp->scope().add(name, out);
            return *out;
        }

        Value& FunctionDef::val(const utils::String& name, DataType* tp) {
            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(this, tp);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            m_comp->scope().add(name, v);
            return *v;
        }
        
        Value& FunctionDef::val(const utils::String& name, u32 module_data_slot) {
            return val(name, m_comp->getOutput()->getModule(), module_data_slot);
        }
        
        Value& FunctionDef::val(const utils::String& name, Module* m, u32 module_data_slot) {
            const module_data& info = m->getDataInfo(module_data_slot);

            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(this, info.type);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            v->m_flags.is_pointer = 1;

            add(ir_module_data).op(*v).op(imm<u32>(m->getId())).op(imm<u32>(module_data_slot));
            m_comp->scope().add(name, v);
            return *v;
        }

        Value FunctionDef::val(Module* m, u32 module_data_slot) {
            const module_data& info = m->getDataInfo(module_data_slot);

            Value v = Value(this, info.type ? info.type : m_comp->getContext()->getTypes()->getType<void*>());
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
                out.setStackSrc(v);
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
        }

        Function* FunctionDef::onExit() {
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

            ffi::FunctionType* sig = new FunctionType(m_thisTp, m_retTp, m_argInfo, false);
            
            bool sigExisted = false;
            const auto& types = m_comp->getOutput()->getTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (!info.is_function) continue;
                if (sig->isEquivalentTo(t)) {
                    delete sig;
                    sig = (FunctionType*)t;
                    sigExisted = true;
                    break;
                }
            }

            if (!sigExisted) {
                const auto& types =  m_comp->getContext()->getTypes()->allTypes();
                for (auto* t : types) {
                    const auto& info = t->getInfo();
                    if (!info.is_function) continue;
                    if (sig->isEquivalentTo(t)) {
                        delete sig;
                        sig = (FunctionType*)t;
                        sigExisted = true;
                        break;
                    }
                }
            }

            if (!sigExisted) {
                m_comp->getOutput()->add(sig);
            }


            m_output = new Function(
                m_name,
                m_comp->getOutput()->getModule()->getName() + "::",
                sig,
                private_access,
                nullptr,
                nullptr,
                m_comp->getOutput()->getModule()
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