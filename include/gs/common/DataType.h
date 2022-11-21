#pragma once
#include <gs/common/types.h>
#include <utils/Array.h>
#include <utils/String.h>

namespace gs {
    namespace compiler {
        struct ast_node;
    };

    namespace ffi {
        class DataType;
        class Function;
        class Method;
        class DataTypeRegistry;

        struct type_property {
            utils::String name;
            access_modifier access;
            u64 offset;

            DataType* type;
            value_flags flags;
            Function* getter;
            Function* setter;
        };

        struct type_base {
            DataType* type;
            u64 offset;
            access_modifier access;
        };

        class DataType {
            public:
                DataType(
                    const utils::String& name,
                    const utils::String& fullyQualifiedName,
                    const type_meta& info
                );
                DataType(
                    const utils::String& name,
                    const utils::String& fullyQualifiedName,
                    const type_meta& info,
                    const utils::Array<type_property>& properties,
                    const utils::Array<type_base>& bases,
                    Function* dtor,
                    const utils::Array<Function*>& methods
                );
                virtual ~DataType();
                
                /**
                 * @brief Find methods that match the provided signature
                 * 
                 * @param name Function name
                 * @param retTp Signature return type
                 * @param argTps Signature argument types
                 * @param argCount Signature argument count
                 * @param flags Search options (See 'function_match_flags' documentation for more info)
                 * @return Subset of methods that match the provided signature
                 */
                utils::Array<ffi::Function*> findMethods(
                    const utils::String& name,
                    const DataType* retTp,
                    const DataType** argTps,
                    u8 argCount,
                    function_match_flags flags
                ) const;

                const type_property* getProp(const utils::String& name, bool excludeInherited = false, bool excludePrivate = false) const;

                type_id getId() const;
                const utils::String& getName() const;
                const utils::String& getFullyQualifiedName() const;
                const type_meta& getInfo() const;
                const utils::Array<type_property>& getProperties() const;
                const utils::Array<Function*>& getMethods() const;
                const utils::Array<type_base>& getBases() const;
                Function* getDestructor() const;
                access_modifier getAccessModifier() const;
                void setAccessModifier(access_modifier access);

                virtual bool isConvertibleTo(const DataType* to) const;
                virtual bool isImplicitlyAssignableTo(const DataType* to) const;
                virtual bool isEquivalentTo(const DataType* to) const;

                DataType* clone(const utils::String& name, const utils::String& fullyQualifiedName) const;
            
            protected:
                DataType();
            
            private:
                friend class FunctionType;
                friend class ClassType;
                friend class DataTypeBinder;
                friend class DataTypeRegistry;
                type_id m_id;
                utils::String m_name;
                utils::String m_fullyQualifiedName;
                type_meta m_info;
                access_modifier m_access;
                utils::Array<type_property> m_properties;
                utils::Array<type_base> m_bases;

                Function* m_destructor;
                utils::Array<Function*> m_methods;
        };

        struct function_argument {
            arg_type argType;
            DataType* dataType;
            bool isImplicit() const;
        };

        class FunctionType : public DataType {
            public:
                FunctionType(DataType* returnType, const utils::Array<function_argument>& args);
                virtual ~FunctionType();

                utils::String generateFullyQualifiedFunctionName(const utils::String& funcName);
                DataType* getReturnType() const;
                const utils::Array<function_argument>& getArguments() const;
                virtual bool isEquivalentTo(DataType* to) const;
            
            protected:
                friend class Function;
                void setThisType(DataType* tp);

            private:
                DataType* m_returnType;
                utils::Array<function_argument> m_args;
        };

        class TemplateType : public DataType {
            public:
                TemplateType(const utils::String& name, const utils::String& fullyQualifiedName, compiler::ast_node* baseAST);
                virtual ~TemplateType();

                compiler::ast_node* getAST() const;

            private:
                compiler::ast_node* m_ast;
        };

        class AliasType : public DataType {
            public:
                AliasType(const utils::String& name, const utils::String& fullyQualifiedName, DataType* refTp);
                virtual ~AliasType();

                DataType* getRefType() const;

            private:
                DataType* m_ref;
        };

        class ClassType : public DataType {
            public:
                ClassType(const utils::String& name, const utils::String& fullyQualifiedName);
                virtual ~ClassType();

                void addBase(DataType* tp, access_modifier access = public_access);
                void addProperty(const utils::String& name, DataType* tp, value_flags flags, access_modifier access = public_access, Function* getter = nullptr, Function* setter = nullptr);
                void addMethod(ffi::Method* method);
                void setDestructor(ffi::Method* dtor);
        };
    };
};