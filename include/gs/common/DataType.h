#pragma once
#include <gs/common/types.h>
#include <utils/Array.h>
#include <utils/String.h>

namespace gs {
    namespace ffi {
        class DataType;
        class Function;

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
                virtual ~DataType();

                type_id getId() const;
                const utils::String& getName() const;
                const utils::String& getFullyQualifiedName() const;
                const type_meta& getInfo() const;
                const utils::Array<type_property>& getProperties() const;
                const utils::Array<type_base>& getBases() const;
                const utils::Array<Function*>& getMethods() const;
                Function* getDestructor() const;

                bool isConvertibleTo(DataType* to) const;
            
            protected:
                DataType();
            
            private:
                friend class FunctionSignatureType;
                friend class DataTypeBinder;
                type_id m_id;
                utils::String m_name;
                utils::String m_fullyQualifiedName;
                type_meta m_info;
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

        class FunctionSignatureType : public DataType {
            public:
                FunctionSignatureType(DataType* returnType, const utils::Array<function_argument>& args);
                virtual ~FunctionSignatureType();

                utils::String generateFullyQualifiedFunctionName(const utils::String& funcName);
                DataType* getReturnType() const;
                const utils::Array<function_argument>& getArguments() const;

            private:
                DataType* m_returnType;
                utils::Array<function_argument> m_args;
        };
    };
};