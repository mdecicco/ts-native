#pragma once
#include <gs/common/types.h>
#include <gs/common/DataType.h>
#include <gs/compiler/IR.h>
#include <gs/interfaces/ICodeHolder.h>

#include <utils/String.h>

namespace gs {
    class Context;

    namespace ffi {
        class Function;
        struct function_argument;
    };

    namespace compiler {
        class Compiler;
        class Value;

        class FunctionDef : public ICodeHolder {
            public:
                ~FunctionDef();

                InstructionRef add(ir_instruction i);
                label_id label();

                Compiler* getCompiler() const;
                Context* getContext() const;

                const utils::String& getName() const;
                void setReturnType(ffi::DataType* tp);
                void setThisType(ffi::DataType* tp);
                ffi::DataType* getReturnType() const;
                ffi::DataType* getThisType() const;

                u32 getArgCount() const;
                u32 getImplicitArgCount() const;
                void addArg(const utils::String& name, ffi::DataType* tp);
                void addDeferredArg(const utils::String& name, ffi::DataType* tp);
                const ffi::function_argument& getArgInfo(u32 argIdx) const;
                const utils::Array<ffi::function_argument>& getArgs() const;
                Value& getArg(u32 argIdx);
                Value& getThis();
                Value& getECtx();
                Value& getFPtr();
                Value& getRetPtr();
                Value& getPoison();

                alloc_id reserveStackId();
                void setStackId(Value& v, alloc_id id);
                
                Value& val(const utils::String& name, ffi::DataType* tp);
                Value& val(const utils::String& name, u32 module_data_slot);
                Value& val(const utils::String& name, Module* m, u32 module_data_slot);
                Value val(Module* m, u32 module_data_slot);
                Value val(ffi::DataType* tp);
                
                template <typename T>
                Value& val(const utils::String& name);

                template <typename T>
                Value val();

                template <typename T>
                std::enable_if_t<is_imm_v<T>, Value>
                imm(T value);

                Value imm(ffi::DataType* tp);

                void onEnter();
                ffi::Function* onExit();

                ffi::Function* getOutput();

            private:
                friend class CompilerOutput;
                FunctionDef(Compiler* c, const utils::String& name, ffi::DataType* methodOf);
                FunctionDef(Compiler* c, ffi::Function* func);

                Compiler* m_comp;
                utils::String m_name;
                ffi::DataType* m_retTp;
                u8 m_implicitArgCount;
                utils::Array<ffi::function_argument> m_argInfo;
                utils::Array<utils::String> m_argNames;
                utils::Array<Value*> m_args;
                Value* m_thisArg;
                Value* m_ectxArg;
                Value* m_fptrArg;
                Value* m_retpArg;
                Value* m_poison;
                ffi::DataType* m_thisTp;
                ffi::Function* m_output;
                
                alloc_id m_nextAllocId;
                vreg_id m_nextRegId;
        };
    };
};