#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/utils/SourceLocation.h>
#include <utils/String.h>

namespace tsn {
    namespace compiler {
        class ParseNode;
        class Compiler;
    };

    namespace ffi {
        class FunctionType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class DataType;

        class Function : public IPersistable {
            public:
                Function();
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

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                friend class compiler::Compiler;
                void setThisType(DataType* tp);
                bool m_isMethod;
                bool m_isTemplate;
                SourceLocation m_src;
            
            private:
                friend class FunctionRegistry;
                function_id m_id;
                u32 m_registryIndex;
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
                Method();
                Method(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset);

                u64 getThisPtrOffset() const;
                Method* clone(const utils::String& name, u64 baseOffset) const;

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            private:
                u64 m_baseOffset;
        };
    
        class TemplateFunction : public Function {
            public:
                TemplateFunction();
                TemplateFunction(const utils::String& name, access_modifier access, compiler::ParseNode* ast);
                ~TemplateFunction();

                compiler::ParseNode* getAST();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            private:
                compiler::ParseNode* m_ast;
        };
    
        class TemplateMethod : public Method {
            public:
                TemplateMethod();
                TemplateMethod(const utils::String& name, access_modifier access, u64 baseOffset, compiler::ParseNode* ast);
                ~TemplateMethod();

                compiler::ParseNode* getAST();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            private:
                compiler::ParseNode* m_ast;
        };
    };
};