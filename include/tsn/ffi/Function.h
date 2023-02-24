#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/utils/SourceLocation.h>
#include <utils/String.h>

namespace tsn {
    namespace compiler {
        class TemplateContext;
        class Compiler;
        class Output;
    };

    namespace ffi {
        class FunctionType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class DataType;

        class Function : public IPersistable {
            public:
                Function();
                Function(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr);
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
                //         Will be null pointer
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
                friend class FunctionRegistry;
                friend class compiler::Compiler;
                friend class compiler::Output;
                void setThisType(DataType* tp, DataTypeRegistry* treg);
                void setRetType(DataType* tp, bool returnsPointer, DataTypeRegistry* treg);
                bool m_isMethod;
                bool m_isTemplate;
                SourceLocation m_src;
                function_id m_id;
                u32 m_registryIndex;
                utils::String m_name;
                utils::String m_displayName;
                utils::String m_fullyQualifiedName;
                utils::String m_extraQualifiers;
                FunctionType* m_signature;
                access_modifier m_access;
                void* m_address;
                void* m_wrapperAddress;
        };

        class Method : public Function {
            public:
                Method();
                Method(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset);

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
                TemplateFunction(const utils::String& name, const utils::String& extraQualifiers, access_modifier access, compiler::TemplateContext* templateData);
                ~TemplateFunction();

                compiler::TemplateContext* getTemplateData();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            private:
                compiler::TemplateContext* m_data;
        };
    
        class TemplateMethod : public Method {
            public:
                TemplateMethod();
                TemplateMethod(const utils::String& name, const utils::String& extraQualifiers, access_modifier access, u64 baseOffset, compiler::TemplateContext* templateData);
                ~TemplateMethod();

                compiler::TemplateContext* getTemplateData();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            private:
                compiler::TemplateContext* m_data;
        };
    };
};