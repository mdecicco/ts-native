#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>

#include <utils/Array.h>
#include <utils/String.h>

namespace tsn {
    namespace compiler {
        class ParseNode;
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

        enum data_type_instance {
            dti_plain,
            dti_function,
            dti_template,
            dti_alias,
            dti_class
        };

        class DataType : public IPersistable {
            public:
                DataType();
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

                /**
                 * @brief Checks if this type is convertible to some data type. A data type
                 * is convertible to another data type if one of the following is true:
                 * 
                 * - Both this type and the other type are primitives
                 * - This type has a cast operator override that returns the other type
                 * - The other type has a constructor which takes exactly one parameter,
                 *   and that parameter's type is this type
                 * 
                 * @return Returns true if this type is convertible to the other type
                 */
                virtual bool isConvertibleTo(const DataType* to) const;

                /**
                 * @brief Checks if this type can be implicitly assigned to objects of another
                 * type. A data type is implicitly assignable to another data type if one of
                 * the following is true:
                 * 
                 * - Both this type and the other type are primitives
                 * - Both this type and the other type are the same trivially copyable type
                 * - Both this type and the other type are equivalent and trivially copyable
                 * 
                 * @return Returns true if this type is implicitly assignable to the other type
                 */
                virtual bool isImplicitlyAssignableTo(const DataType* to) const;

                /**
                 * @brief Checks if this type is equivalent to some data type. A data type is
                 * equivalent to another data type if all of the following are true:
                 * 
                 * - All of the following attributes for both types must be the same
                 *     - is_pod
                 *     - is_trivially_constructible 
                 *     - is_trivially_copyable
                 *     - is_trivially_destructible
                 *     - is_primitive
                 *     - is_floating_point
                 *     - is_integral
                 *     - is_unsigned
                 *     - is_function
                 *     - is_template
                 * - Both types must have the same number of methods
                 * - Every method on one type must have a counterpart on the other type which has
                 *     - The same name
                 *     - The same signature
                 *     - The same access modifier
                 *     - The same result from ffi::Function::isMethod
                 * - Both types must have the same number of properties
                 * - Every property on one type must have a counterpart on the other type which has
                 *     - The same name
                 *     - The same offset
                 *     - The same access modifier
                 *     - The same flags
                 *     - The same type
                 * - Both types must inherit from the same types in the same order, or both must not
                 *   inherit from any base types
                 *   
                 * @return Returns true if this type is equivalent to the other type
                 */
                virtual bool isEquivalentTo(const DataType* to) const;

                /**
                 * @brief Checks if this type is equal to some data type. A data type is equal to
                 * another data type if both of their effective types have the same ID.
                 * 
                 * @return Returns true if both data types are the same
                 */
                bool isEqualTo(const DataType* to) const;

                DataType* clone(const utils::String& name, const utils::String& fullyQualifiedName) const;

                /**
                 * @brief Follows the chain of aliases to get the data type being referred to.
                 * 
                 * @return The effective data type that this data type refers to, if this data
                 *         type is an alias. Otherwise this function returns this data type.
                 */
                const DataType* getEffectiveType() const;

                /**
                 * @brief Follows the chain of aliases to get the data type being referred to.
                 * 
                 * @return The effective data type that this data type refers to, if this data
                 *         type is an alias. Otherwise this function returns this data type.
                 */
                DataType* getEffectiveType();

                /**
                 * @brief Get which type of data type this is
                 */
                data_type_instance getInstanceType() const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);
            
            protected:
                friend class DataTypeBinder;
                friend class DataTypeRegistry;
                friend class FunctionType;
                friend class TemplateType;
                friend class AliasType;
                friend class ClassType;
                
                type_id m_id;
                data_type_instance m_itype;
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
                FunctionType();
                FunctionType(DataType* returnType, const utils::Array<function_argument>& args);
                virtual ~FunctionType();

                utils::String generateFullyQualifiedFunctionName(const utils::String& funcName);
                utils::String generateFunctionDisplayName(const utils::String& funcName);
                DataType* getReturnType() const;
                const utils::Array<function_argument>& getArguments() const;
                virtual bool isEquivalentTo(DataType* to) const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);
            protected:
                friend class Function;
                void setThisType(DataType* tp);

            private:
                DataType* m_returnType;
                utils::Array<function_argument> m_args;
        };

        class TemplateType : public DataType {
            public:
                TemplateType();
                TemplateType(const utils::String& name, const utils::String& fullyQualifiedName, compiler::ParseNode* baseAST);
                virtual ~TemplateType();

                compiler::ParseNode* getAST() const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);

            private:
                compiler::ParseNode* m_ast;
        };

        class AliasType : public DataType {
            public:
                AliasType();
                AliasType(const utils::String& name, const utils::String& fullyQualifiedName, DataType* refTp);
                virtual ~AliasType();

                DataType* getRefType() const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);

            private:
                DataType* m_ref;
        };

        class ClassType : public DataType {
            public:
                ClassType();
                ClassType(const utils::String& name, const utils::String& fullyQualifiedName);
                virtual ~ClassType();

                void addBase(DataType* tp, access_modifier access = public_access);
                void addProperty(const utils::String& name, DataType* tp, value_flags flags, access_modifier access = public_access, Function* getter = nullptr, Function* setter = nullptr);
                void addMethod(ffi::Method* method);
                void setDestructor(ffi::Method* dtor);
        };
    };
};