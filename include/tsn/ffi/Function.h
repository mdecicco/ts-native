#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/utils/SourceLocation.h>
#include <utils/String.h>

namespace tsn {
    class Module;

    namespace compiler {
        class TemplateContext;
        class Compiler;
        class Output;
        struct InlineCodeGenContext;
        typedef void (*InlineCodeGenFunc)(compiler::InlineCodeGenContext* ctx);
    };

    namespace ffi {
        class FunctionType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class DataType;

        class FunctionPointer {
            public:
                class _undefinedClass;
                using FuncPtrTp = void (*)();
                using MethodPtrTp = void(_undefinedClass::*)();

                FunctionPointer();
                FunctionPointer(nullptr_t);
                FunctionPointer(const FunctionPointer& ptr);
                FunctionPointer(void* ptr, size_t size);

                template <typename Ret, typename... Args>
                FunctionPointer(Ret (*ptr)(Args...));

                template <typename Cls, typename Ret, typename... Args>
                FunctionPointer(Ret (Cls::*ptr)(Args...));

                template <typename Cls, typename Ret, typename... Args>
                FunctionPointer(Ret (Cls::*ptr)(Args...) const);

                template <typename Ret, typename... Args>
                void get(Ret (**ptr)(Args...)) const;

                template <typename Cls, typename Ret, typename... Args>
                void get(Ret (Cls::**ptr)(Args...)) const;

                template <typename Cls, typename Ret, typename... Args>
                void get(Ret (Cls::**ptr)(Args...) const) const;

                void operator=(const FunctionPointer& ptr);

                bool isValid() const;

            protected:
                u8 m_data[32];
                bool m_isSet;
        };
        
        class Function : public IPersistable {
            public:
                Function();
                Function(
                    const utils::String& name,
                    const utils::String& extraQualifiers,
                    FunctionType* signature,
                    access_modifier access,
                    const FunctionPointer& address,
                    const FunctionPointer& wrapperAddr,
                    Module* source,
                    DataType* owner
                );
                virtual ~Function();

                function_id getId() const;
                Module* getSource() const;
                const utils::String& getName() const;
                const utils::String& getDisplayName() const;
                const utils::String& getFullyQualifiedName() const;
                FunctionType* getSignature() const;
                DataType* getOwner() const;
                const SourceLocation& getSourceLocation() const;
                access_modifier getAccessModifier() const;
                void setAccessModifier(access_modifier access);
                const function_flags& getFlags() const;
                function_flags& getFlags();

                void makeInline(compiler::InlineCodeGenFunc generatorFn);
                
                const FunctionPointer& getAddress() const;
                const FunctionPointer& getWrapperAddress() const;

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                friend class FunctionRegistry;
                friend class DataTypeBinder;
                friend class DataTypeExtender;
                friend class compiler::Compiler;
                friend class compiler::Output;
                void setThisType(DataType* tp, DataTypeRegistry* treg);
                void setRetType(DataType* tp, bool returnsPointer, DataTypeRegistry* treg);
                function_flags m_flags;
                SourceLocation m_src;
                function_id m_id;
                Module* m_sourceModule;
                u32 m_registryIndex;
                utils::String m_name;
                utils::String m_displayName;
                utils::String m_fullyQualifiedName;
                utils::String m_extraQualifiers;
                FunctionType* m_signature;
                DataType* m_owner;
                access_modifier m_access;
                FunctionPointer m_address;
                FunctionPointer m_wrapperAddress;
        };

        class Method : public Function {
            public:
                Method();
                Method(
                    const utils::String& name,
                    const utils::String& extraQualifiers,
                    FunctionType* signature,
                    access_modifier access,
                    const FunctionPointer& address,
                    const FunctionPointer& wrapperAddr,
                    u64 baseOffset,
                    Module* source,
                    DataType* owner
                );

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
                TemplateFunction(
                    const utils::String& name,
                    const utils::String& extraQualifiers,
                    access_modifier access,
                    compiler::TemplateContext* templateData,
                    DataType* owner
                );
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
                TemplateMethod(
                    const utils::String& name,
                    const utils::String& extraQualifiers,
                    access_modifier access,
                    u64 baseOffset,
                    compiler::TemplateContext* templateData,
                    DataType* owner
                );
                ~TemplateMethod();

                compiler::TemplateContext* getTemplateData();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            private:
                compiler::TemplateContext* m_data;
        };
    };
};