#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/Output.h>
#include <tsn/common/Context.h>
#include <tsn/common/Function.h>
#include <tsn/common/DataType.h>
#include <tsn/common/Module.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/interfaces/ICodeHolder.h>
#include <tsn/bind/ExecutionContext.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

using namespace utils;
using namespace tsn::ffi;

namespace tsn {
    namespace compiler {
        FunctionDef::FunctionDef(Compiler* c, const utils::String& name, DataType* methodOf) {
            m_comp = c;
            m_src = c->getCurrentSrc();
            m_name = name;
            m_retTp = c->getContext()->getTypes()->getType<void>();
            m_retTpSet = false;
            m_thisTp = methodOf;
            m_output = nullptr;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_fptrArg = nullptr;
            m_retpArg = nullptr;
            m_poison = &val("@poison", c->getContext()->getTypes()->getType<poison_t>());
            
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();
            m_argInfo.push({ arg_type::func_ptr, ptrTp });
            m_argInfo.push({ arg_type::ret_ptr, m_retTp });
            m_argInfo.push({ arg_type::context_ptr, ptrTp });
            if (methodOf) {
                m_argInfo.push({ arg_type::this_ptr, methodOf });
                m_implicitArgCount = 4;
            } else m_implicitArgCount = 3;
        }
        
        FunctionDef::FunctionDef(Compiler* c, Function* func) {
            m_comp = c;
            m_src = c->getCurrentSrc();
            m_name = func->getName();
            m_thisTp = nullptr;
            m_retTp = c->getContext()->getTypes()->getType<void>();
            m_retTpSet = false;
            m_output = func;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_fptrArg = nullptr;
            m_retpArg = nullptr;
            m_poison = &val("@poison", c->getContext()->getTypes()->getType<poison_t>());
            m_implicitArgCount = 0;

            FunctionType* sig = func->getSignature();
            if (sig) {
                m_retTp = sig->getReturnType();
                m_retTpSet = true;
                const auto& args = sig->getArguments();
                for (u8 a = 0;a < args.size();a++) {
                    if (args[a].isImplicit()) m_implicitArgCount++;
                    m_argInfo.push(args[a]);
                }
            }
        }

        FunctionDef::~FunctionDef() {
            if (m_fptrArg) delete m_fptrArg;
            m_fptrArg = nullptr;

            if (m_ectxArg) delete m_ectxArg;
            m_ectxArg = nullptr;

            if (m_ectxArg) delete m_ectxArg;
            m_ectxArg = nullptr;

            if (m_thisArg) delete m_thisArg;
            m_thisArg = nullptr;

            for (u8 a = 0;a < m_args.size();a++) {
                delete m_args[a];
                m_args[a] = nullptr;
            }
        }

        InstructionRef FunctionDef::add(ir_instruction i) {
            return ICodeHolder::add(i, m_comp->getCurrentSrc());
        }

        label_id FunctionDef::label() {
            return ICodeHolder::label(m_comp->getCurrentSrc());
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
        }

        void FunctionDef::setThisType(ffi::DataType* tp) {
            bool found = false;
            for (u8 a = 0;a < m_argInfo.size();a++) {
                if (m_argInfo[a].argType == arg_type::this_ptr) {
                    found = true;
                    m_argInfo[a].dataType = tp;
                }
            }

            if (found) m_thisTp = tp;
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
            return m_argInfo.size() - m_implicitArgCount;
        }

        u32 FunctionDef::getImplicitArgCount() const {
            return m_implicitArgCount;
        }

        void FunctionDef::addArg(const utils::String& name, DataType* tp) {
            m_argNames.push(name);
            m_argInfo.push({
                tp->getInfo().is_primitive ? arg_type::value : arg_type::pointer,
                tp
            });

            Value& s = val(name, tp);
            s.m_flags.is_argument = 1;
            s.m_imm.u = m_args.size();
            m_args.push(new Value(s));
        }

        void FunctionDef::addDeferredArg(const utils::String& name, DataType* tp) {
            m_argNames.push(name);

            Value& s = val(name, tp);
            s.m_flags.is_argument = 1;
            s.m_imm.u = m_args.size();
            m_args.push(new Value(s));
        }
        
        const function_argument& FunctionDef::getArgInfo(u32 argIdx) const {
            return m_argInfo[argIdx + m_implicitArgCount];
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

        Value& FunctionDef::getECtx() {
            return *m_ectxArg;
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

            Value v = Value(this, info.type);
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

        Value FunctionDef::imm(ffi::DataType* tp) {
            return Value(this, tp, true);
        }

        void FunctionDef::onEnter() {
            DataType* voidp = m_comp->getContext()->getTypes()->getType<void*>();
            DataType* ectx = m_comp->getContext()->getTypes()->getType<ExecutionContext>();
            DataType* errt = m_comp->getContext()->getTypes()->getType<poison_t>();

            Value& f = val("@fptr", voidp);
            f.m_flags.is_argument = 1;
            f.m_imm.u = 0;
            m_fptrArg = new Value(f);

            Value& r = val("@ret", voidp);
            r.m_flags.is_argument = 1;
            r.m_imm.u = 1;
            m_retpArg = new Value(r);

            Value& c = val("@ectx", ectx);
            c.m_flags.is_argument = 1;
            c.m_imm.u = 2;
            m_ectxArg = new Value(c);

            Value& p = val("@poison", errt);

            if (m_thisTp) {
                Value& t = val("this", m_thisTp);
                t.m_flags.is_argument = 1;
                t.m_flags.is_pointer = true;
                t.m_imm.u = 3;
                m_thisArg = new Value(t);
            }
        }

        Function* FunctionDef::onExit() {
            if (!m_retTp) setReturnType(m_comp->getContext()->getTypes()->getType<void>());
            if (m_output) return m_output;
            if (m_instructions.size() == 0) return nullptr;

            m_output = new Function(
                m_name,
                new FunctionType(m_retTp, m_argInfo),
                private_access,
                nullptr,
                nullptr
            );

            m_comp->getOutput()->resolveFunctionDef(this, m_output);

            return m_output;
        }
        
        ffi::Function* FunctionDef::getOutput() {
            return m_output;
        }
        
        bool FunctionDef::serialize(utils::Buffer* out, Context* ctx) const {
            auto writeInstruction = [out, ctx](const Instruction& i) {
                return !i.serialize(out, ctx);
            };

            if (!out->write(m_output->isMethod())) return false;
            if (!out->write(m_output->isTemplate())) return false;
            if (!m_output->serialize(out, ctx)) return false;
            if (!out->write(m_instructions.size())) return false;
            if (m_instructions.some(writeInstruction)) return false;
            
            return true;
        }

        bool FunctionDef::deserialize(utils::Buffer* in, Context* ctx) {
            bool isMethod = false, isTemplate = false;
            if (!in->read(isMethod)) return false;
            if (!in->read(isTemplate)) return false;

            if (isTemplate) {
                if (isMethod) m_output = new TemplateMethod();
                else m_output = new TemplateFunction();
            } else {
                if (isMethod) m_output = new Method();
                else m_output = new Function();
            }

            if (!m_output->deserialize(in, ctx)) {
                delete m_output;
                m_output = nullptr;
                return false;
            }

            u32 instructionCount = 0;
            if (!in->read(instructionCount)) return false;
            for (u32 i = 0;i < instructionCount;i++) {
                Instruction inst;
                if (!inst.deserialize(in, ctx)) return false;
                m_instructions.push(inst);
            }

            return true;
        }
    };
};