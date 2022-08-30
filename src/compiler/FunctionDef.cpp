#include <gs/compiler/FunctionDef.h>
#include <gs/compiler/Compiler.h>
#include <gs/common/Context.h>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/interfaces/ICodeHolder.h>

#include <utils/Array.hpp>

using namespace utils;
using namespace gs::ffi;

namespace gs {
    namespace compiler {
        FunctionDef::FunctionDef(Compiler* c, const utils::String& name, ffi::DataType* methodOf) {
            m_comp = c;
            m_name = name;
            m_retTp = nullptr;
            m_thisTp = methodOf;
            m_output = nullptr;
            m_nextAllocId = 1;
            m_nextRegId = 1;
            
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();
            // ret pointer type will be set later
            m_argInfo.push({ arg_type::ret_ptr, nullptr });
            m_argInfo.push({ arg_type::context_ptr, ptrTp });
            if (methodOf) m_argInfo.push({ arg_type::this_ptr, methodOf });
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

        void FunctionDef::setReturnType(ffi::DataType* tp) {
            m_retTp = tp;
            m_argInfo[0].dataType = tp;
        }

        ffi::DataType* FunctionDef::getReturnType() const {
            return m_retTp;
        }

        ffi::DataType* FunctionDef::getThisType() const {
            return m_thisTp;
        }

        u32 FunctionDef::getArgCount() const {
            return m_args.size();
        }

        void FunctionDef::addArg(const utils::String& name, ffi::DataType* tp) {
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

        Value& FunctionDef::val(const utils::String& name, ffi::DataType* tp) {
            // Must be dynamically allocated to be stored in symbol table
            Value* v = new Value(m_comp, tp);
            v->m_regId = m_nextRegId++;
            v->m_name = name;
            m_comp->scope().get().add(name, v);
            return *v;
        }

        Value FunctionDef::val(ffi::DataType* tp) {
            Value v = Value(m_comp, tp);
            v.m_regId = m_nextRegId++;
            return v;
        }

        ffi::Function* FunctionDef::getOutput() const {
            return m_output;
        }
    };
};