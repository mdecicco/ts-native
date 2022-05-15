#include <gs/common/DataType.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        //
        // DataType
        //

        DataType::DataType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            const type_meta& info
        ) {
            m_id = std::hash<utils::String>()(fullyQualifiedName);
            m_name = name;
            m_fullyQualifiedName = fullyQualifiedName;
            m_info = info;
            m_destructor = nullptr;
        }

        DataType::DataType() {
            m_id = 0;
        }

        DataType::~DataType() {
        }

        type_id DataType::getId() const {
            return m_id;
        }

        const utils::String& DataType::getName() const {
            return m_name;
        }

        const type_meta& DataType::getInfo() const {
            return m_info;
        }

        const utils::Array<type_property>& DataType::getProperties() const {
            return m_properties;
        }

        const utils::Array<type_base>& DataType::getBases() const {
            return m_bases;
        }

        const utils::Array<Function*>& DataType::getMethods() const {
            return m_methods;
        }

        Function* DataType::getDestructor() const {
            return m_destructor;
        }

        bool DataType::isConvertibleTo(DataType* to) const {
            return false;
        }



        //
        // function_argument
        //

        bool function_argument::isImplicit() const {
            return (u32)argType <= (u32)arg_type::this_ptr;
        }


        //
        // FunctionSignatureType
        //
        
        FunctionSignatureType::FunctionSignatureType(DataType* returnType, const utils::Array<function_argument>& args) {
            m_name = returnType->m_name + "(";
            m_fullyQualifiedName = returnType->m_fullyQualifiedName + "(";
            args.each([this](const function_argument& arg, u32 idx) {
                if (idx > 0) {
                    m_name += ",";
                    m_fullyQualifiedName += ",";
                }

                bool is_implicit = arg.argType == arg_type::func_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::ret_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::context_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::this_ptr;
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) {
                    m_name += "$";
                    m_fullyQualifiedName += "$";
                }

                m_name += arg.dataType->m_name;
                m_fullyQualifiedName += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) {
                    m_name += "*";
                    m_fullyQualifiedName += "*";
                }
            });

            m_name += ")";
            m_fullyQualifiedName += ")";

            m_id = std::hash<utils::String>()(m_fullyQualifiedName);
            m_info = {
                true         , // is pod
                false        , // is_trivially_copyable
                false        , // is_primitive
                false        , // is_floating_point
                false        , // is_unsigned
                true         , // is_function
                true         , // is_host
                sizeof(void*), // size
                0              // host_hash
            };

            m_returnType = returnType;
            m_args = args;
        }

        FunctionSignatureType::~FunctionSignatureType() {
        }

        utils::String FunctionSignatureType::generateFullyQualifiedFunctionName(const utils::String& funcName) {
            utils::String name = m_returnType->m_fullyQualifiedName + " " + funcName + "(";
            m_args.each([&name](const function_argument& arg, u32 idx) {
                if (idx > 0) name += ",";

                bool is_implicit = arg.argType == arg_type::this_ptr || arg.argType == arg_type::context_ptr;
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) name += "$";

                name += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) name += "*";
            });

            name += ")";

            return name;
        }

        DataType* FunctionSignatureType::getReturnType() const {
            return m_returnType;
        }

        const utils::Array<function_argument>& FunctionSignatureType::getArguments() const {
            return m_args;
        }
    };
};