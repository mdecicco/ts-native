#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>

#include <utils/Array.h>
#include <utils/String.h>

namespace tsn {
    class Pipeline;
    class Module;

    namespace compiler {
        class TemplateContext;
        class Output;
        class Compiler;
    };

    namespace ffi {
        class DataType;
        class TemplateType;
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

        enum data_type_instance : u32 {
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
                Module* getSource() const;
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

                /**
                 * @brief Checks if this type is able to be constructed with the provided argument types
                 * 
                 * @return Returns true if this type is constructable with the provided argument types
                 */
                bool isConstructableWith(const utils::Array<DataType*>& args) const;

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

                /**
                 * @brief Get the template type that this type's effective type is an instantiation
                 *        of, if this type's effective type is an instantiation of a template type.
                 * 
                 * @return The template type that this type's effective type was instantiated from,
                 *         or nullptr if this type's effective type is not a template instantiation.
                 */
                ffi::TemplateType* getTemplateBase() const;

                /**
                 * @brief Get the template arguments that were used to instantiate this type's
                 *        effective type, if this type's effective type is an instantiation
                 *        of a template type.
                 * 
                 * @return Array of ffi::DataType* that were used as arguments to instantiate the
                 *         template. If this is not a template instantiation, it will be an empty
                 *         array.
                 */
                const utils::Array<DataType*>& getTemplateArguments() const;

                /**
                 * @brief Checks if this type's effective type is an instantiation of the specified
                 *        template type
                 * 
                 * @param templ The template type to check if this type is an instantiation of
                 * 
                 * @return true if this type's effective type is an instantiation of the specified
                 *         type, otherwise false
                 */
                bool isSpecializationOf(ffi::TemplateType* templ) const;
                
                /**
                 * @brief Checks if this type's effective type is a specialization of the specified
                 *        template type with the provided template arguments
                 * 
                 * @param templ    The template type to check if this type is a specialization of
                 * @param withArgs An array of template arguments to check against any template
                 *                 arguments that may have been used to specialize this type
                 *                 from a template.
                 * 
                 * @return true if this type's effective type is a specialization of the specified
                 *         type which using the specified template arguments, otherwise false
                 */
                bool isSpecializationOf(ffi::TemplateType* templ, const utils::Array<ffi::DataType*>& withArgs) const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);
            
            protected:
                friend class DataTypeBinder;
                friend class DataTypeExtender;
                friend class DataTypeRegistry;
                friend class FunctionType;
                friend class TemplateType;
                friend class AliasType;
                friend class ClassType;
                friend class tsn::Pipeline;
                friend class compiler::Output;
                friend class compiler::Compiler;
                
                type_id m_id;
                data_type_instance m_itype;
                Module* m_sourceModule;
                utils::String m_name;
                utils::String m_fullyQualifiedName;
                type_meta m_info;
                access_modifier m_access;
                utils::Array<type_property> m_properties;
                utils::Array<type_base> m_bases;
                ffi::TemplateType* m_templateBase;
                utils::Array<ffi::DataType*> m_templateArgs;

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
                FunctionType(DataType* thisType, DataType* returnType, const utils::Array<function_argument>& args, bool returnsPointer);
                virtual ~FunctionType();

                utils::String generateFullyQualifiedFunctionName(const utils::String& funcName, const utils::String& extraQualifiers);
                utils::String generateFunctionDisplayName(const utils::String& funcName, const utils::String& extraQualifiers);
                DataType* getReturnType() const;
                DataType* getThisType() const;
                bool returnsPointer() const;
                const utils::Array<function_argument>& getArguments() const;
                virtual bool isEquivalentTo(DataType* to) const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                friend class Function;
                friend class compiler::Output;

                DataType* m_returnType;
                DataType* m_thisType;
                bool m_returnsPointer;
                utils::Array<function_argument> m_args;
        };

        class TemplateType : public DataType {
            public:
                TemplateType();
                TemplateType(const utils::String& name, const utils::String& fullyQualifiedName, compiler::TemplateContext* templateData);
                virtual ~TemplateType();

                compiler::TemplateContext* getTemplateData() const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                friend class compiler::Output;
                compiler::TemplateContext* m_data;
        };

        class AliasType : public DataType {
            public:
                AliasType();
                AliasType(const utils::String& name, const utils::String& fullyQualifiedName, DataType* refTp);
                virtual ~AliasType();

                DataType* getRefType() const;
            
                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                friend class compiler::Output;
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