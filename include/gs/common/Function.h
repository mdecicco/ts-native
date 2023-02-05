#pragma once
#include <gs/common/types.h>
#include <gs/utils/SourceLocation.h>
#include <utils/String.h>

namespace gs {
    namespace compiler {
        struct ast_node;
        class Compiler;
    };

    namespace ffi {
        class FunctionType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class DataType;

        class Function {
            public:
                Function(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr);
                virtual ~Function();

                function_id getId() const;
                const utils::String& getName() const;
                const utils::String& getDisplayName() const;
                const utils::String& getFullyQualifiedName() const;
                FunctionType* getSignature() const;
                const SourceLocation& getSource() const;
                access_modifier getAccessModifier() const;
                void setAccessModifier(access_modifier access);
                bool isMethod() const;
                bool isTemplate() const;
                bool isThisCall() const;
                
                // Signature:
                // If host function:
                //     If method of class:
                //         RetTp (ThisTp::*)(ArgTypes...)
                //     If normal function:
                //         RetTp (*)(ArgTypes...)
                // If script function:
                //     If function was JIT compiled:
                //         Same as host function signature
                //     If function runs in a VM:
                //         Not a function pointer, but an index into the instruction buffer
                //         The VM handles translating arguments to this function
                void* getAddress() const;

                // Signature:
                // If method of class
                //     RetTp (RetTp (ThisTp::*)(ArgTypes...), RetTp*, ExecutionContext*, ThisTp*, ArgsTypes...)
                // If normal function
                //     RetTp (RetTp (*)(ArgTypes...), RetTp*, ExecutionContext*, ArgsTypes...)
                // If script function
                //     Will be null pointer
                void* getWrapperAddress() const;

            protected:
                friend class compiler::Compiler;
                void setThisType(DataType* tp);
                bool m_isMethod;
                bool m_isTemplate;
                SourceLocation m_src;
            
            private:
                friend class FunctionRegistry;
                function_id m_id;
                utils::String m_name;
                utils::String m_displayName;
                utils::String m_fullyQualifiedName;
                FunctionType* m_signature;
                access_modifier m_access;
                void* m_address;
                void* m_wrapperAddress;
        };

        class Method : public Function {
            public:
                Method(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset);

                u64 getThisPtrOffset() const;
                Method* clone(const utils::String& name, u64 baseOffset) const;

            private:
                u64 m_baseOffset;
        };
    
        class TemplateFunction : public Function {
            public:
                TemplateFunction(const utils::String& name, access_modifier access, compiler::ast_node* ast);
                ~TemplateFunction();

                compiler::ast_node* getAST();
            
            private:
                compiler::ast_node* m_ast;
        };
    
        class TemplateMethod : public Method {
            public:
                TemplateMethod(const utils::String& name, access_modifier access, u64 baseOffset, compiler::ast_node* ast);
                ~TemplateMethod();

                compiler::ast_node* getAST();
            
            private:
                compiler::ast_node* m_ast;
        };
    };
};