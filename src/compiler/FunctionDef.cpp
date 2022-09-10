#include <gs/compiler/FunctionDef.h>
#include <gs/compiler/Compiler.h>
#include <gs/common/Context.h>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/interfaces/ICodeHolder.h>
#include <gs/bind/ExecutionContext.h>

#include <utils/Array.hpp>

using namespace utils;
using namespace gs::ffi;

namespace gs {
    namespace compiler {
        FunctionDef::FunctionDef(Compiler* c, const utils::String& name, DataType* methodOf) {
            m_comp = c;
            m_name = name;
            m_retTp = nullptr;
            m_thisTp = methodOf;
            m_output = nullptr;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_retpArg = nullptr;
            m_poison = nullptr;
            
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();
            m_argInfo.push({ arg_type::func_ptr, ptrTp });
            // ret pointer type will be set later
            m_argInfo.push({ arg_type::ret_ptr, nullptr });
            m_argInfo.push({ arg_type::context_ptr, ptrTp });
            if (methodOf) m_argInfo.push({ arg_type::this_ptr, methodOf });
        }
        
        FunctionDef::FunctionDef(Compiler* c, Function* func) {
            m_comp = c;
            m_name = func->getName();
            FunctionType* sig = func->getSignature();
            m_retTp = sig->getReturnType();
            m_thisTp = nullptr;
            m_output = func;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            m_thisArg = nullptr;
            m_ectxArg = nullptr;
            m_retpArg = nullptr;
            m_poison = nullptr;

            // explicit arguments will be added externally
            const auto& args = sig->getArguments();
            for (u8 a = 0;a < args.size() && args[a].isImplicit();a++) {
                m_argInfo.push(args[a]);

                if (args[a].argType == arg_type::this_ptr) {
                    m_thisTp = args[a].dataType;
                }
            }
        }

        FunctionDef::~FunctionDef() {
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
        }

        DataType* FunctionDef::getReturnType() const {
            return m_retTp;
        }

        DataType* FunctionDef::getThisType() const {
            return m_thisTp;
        }

        u32 FunctionDef::getArgCount() const {
            return m_args.size();
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
            m_args.push(&s);
        }

        Value& FunctionDef::getArg(u32 argIdx) {
            return *m_args[argIdx];
        }

        Value& FunctionDef::getThis() {
            if (!m_thisArg) throw std::exception("Function is not a class method");
            return *m_thisArg;
        }

        Value& FunctionDef::getECtx() {
            return *m_ectxArg;
        }

        Value& FunctionDef::getRetPtr() {
            return *m_retpArg;
        }

        Value& FunctionDef::getPoison() {
            return *m_poison;
        }

        Value& FunctionDef::val(const utils::String& name, DataType* tp) {
            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(this, tp);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            m_comp->scope().add(name, v);
            return *v;
        }

        Value FunctionDef::val(DataType* tp) {
            Value v = Value(this, tp);
            v.m_regId = m_nextRegId++;
            return v;
        }

        Function* FunctionDef::getOutput() const {
            return m_output;
        }

        void FunctionDef::onEnter() {
            DataType* voidp = m_comp->getContext()->getTypes()->getType<void*>();
            DataType* ectx = m_comp->getContext()->getTypes()->getType<ExecutionContext>();
            DataType* errt = m_comp->getContext()->getTypes()->getType<poison_t>();

            Value& s = val("@ret", voidp);
            s.m_flags.is_argument = 1;
            s.m_imm.u = 1;
            m_ectxArg = &s;

            s = val("@ectx", ectx);
            s.m_flags.is_argument = 1;
            s.m_imm.u = 2;
            m_ectxArg = &s;

            s = val("@poison", errt);

            if (m_thisTp) {
                s = val("this", m_thisTp);
                s.m_flags.is_argument = 1;
                s.m_imm.u = 3;
                m_thisArg = &s;
            }
        }
    };
};