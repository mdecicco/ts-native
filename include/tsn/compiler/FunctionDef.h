#pragma once
#include <tsn/common/types.h>
#include <tsn/ffi/DataType.h>
#include <tsn/compiler/IR.h>

#include <utils/String.h>

namespace tsn {
    class Context;

    namespace ffi {
        class Function;
        struct function_argument;
    };

    namespace compiler {
        class Compiler;
        class Value;

        class FunctionDef {
            public:
                struct capture_info {
                    // Offset into capture data
                    u32 offset;

                    // Address of the last instruction before the
                    // variable was marked as captured, before which
                    // the vreg does not need to be kept in sync with
                    // the capture data
                    u32 firstCapturedAt;

                    // Pointer to the named value that was captured
                    Value* decl;
                };

                ~FunctionDef();

                InstructionRef add(ir_instruction i);
                label_id label();

                Compiler* getCompiler() const;
                Context* getContext() const;

                const utils::String& getName() const;
                void setReturnType(ffi::DataType* tp);
                void setThisType(ffi::DataType* tp);
                ffi::DataType* getReturnType() const;
                bool isReturnTypeExplicit() const;
                ffi::DataType* getThisType() const;
                const SourceLocation& getSource() const;

                u32 getArgCount() const;
                void addArg(const utils::String& name, ffi::DataType* tp);
                void addDeferredArg(const utils::String& name);
                const ffi::function_argument& getArgInfo(u32 argIdx) const;
                const utils::Array<ffi::function_argument>& getArgs() const;
                Value& getArg(u32 argIdx);
                Value& getThis();
                Value& getCCtx();
                Value& getECtx();
                Value& getCaptures();
                Value& getFPtr();
                Value& getRetPtr();
                Value& getPoison();
                Value& getOwnCaptureData();
                Value getNull();
                
                u32 capture(const Value& val);

                alloc_id reserveStackId();
                void setStackId(Value& v, alloc_id id);

                Value& promote(const Value& val, const utils::String& name, Scope* scope = nullptr);
                Value& val(const utils::String& name, ffi::DataType* tp, Scope* scope = nullptr);
                Value& val(const utils::String& name, u32 module_data_slot, Scope* scope = nullptr);
                Value& val(const utils::String& name, Module* m, u32 module_data_slot, Scope* scope = nullptr);
                Value val(Module* m, u32 module_data_slot);
                Value val(ffi::DataType* tp);

                // If 'true' is passed for 'unscoped' then the raw stack value
                // is returned (not pointer to the stack space), and the value
                // is not added to the scope. The stack slot will not be freed
                // by the end of the scope (unless it is added to the scope or
                // freed manually)
                Value stack(
                    ffi::DataType* tp,
                    bool unscoped = false,
                    const utils::String& allocComment = utils::String(),
                    const utils::String& getPtrComment = utils::String()
                );

                template <typename T>
                Value stack(
                    bool unscoped = false,
                    const utils::String& allocComment = utils::String(),
                    const utils::String& getPtrComment = utils::String()
                );
                
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
                const utils::Array<Instruction>& getCode() const;
                utils::Array<Instruction>& getCode();
                ParseNode* getNode();

            private:
                friend class OutputBuilder;
                friend class InstructionRef;

                FunctionDef();
                FunctionDef(Compiler* c, const utils::String& name, ffi::DataType* methodOf, ParseNode* n);
                FunctionDef(Compiler* c, ffi::Function* func, ParseNode* n);

                ParseNode* m_node;
                Value* m_scopeRef;
                SourceLocation m_src;
                Compiler* m_comp;
                utils::String m_name;
                ffi::DataType* m_retTp;
                bool m_retTpSet;
                utils::Array<ffi::function_argument> m_argInfo;
                utils::Array<utils::String> m_argNames;
                utils::Array<Value*> m_args;
                Value* m_cctxArg;
                Value* m_thisArg;
                Value* m_ectxArg;
                Value* m_capsArg;
                Value* m_fptrArg;
                Value* m_retpArg;
                Value* m_ownCaptureData;
                Value* m_poison;
                ffi::DataType* m_thisTp;
                ffi::Function* m_output;

                u32 m_captureDataInsertAddr;
                u32 m_captureDataOffset;
                utils::Array<capture_info> m_captures;

                label_id m_nextLabelId;
                alloc_id m_nextAllocId;
                vreg_id m_nextRegId;
                utils::Array<Instruction> m_instructions;
        };
    };
};