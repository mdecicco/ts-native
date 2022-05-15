#include <gs/bind/bind_type.h>
#include <gs/bind/bind.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/Module.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        DataTypeBinder::DataTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName, type_meta&& meta) {
            funcRegistry = freg;
            typeRegistry = treg;
            m_type = new DataType(name, fullyQualifiedName, meta);
            typeRegistry->addHostType(meta.host_hash, m_type);
            m_isFinal = false;
        }

        DataTypeBinder::~DataTypeBinder() {
            if (!m_isFinal && m_type) {
                typeRegistry->removeType(m_type);
                delete m_type;
            }
            m_type = nullptr;
        }

        void DataTypeBinder::setDestructor(Function* dtor) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_destructor = dtor;
        }

        void DataTypeBinder::addMethod(Function* method) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_methods.push(method);
        }

        void DataTypeBinder::addProperty(type_property&& prop) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_properties.push(prop);
        }
        
        bool DataTypeBinder::propNameExists(const utils::String& name) const {
            return !m_type->m_properties.some([name](const type_property& p) {
                return p.name == name;
            });
        }

        DataType* DataTypeBinder::finalize(Module* mod) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Type definition '%s' is already finalized",
                    m_type->m_name.c_str()
                ));
            }

            if (!m_type->m_info.is_primitive && !m_type->m_info.is_trivially_destructible && !m_type->m_destructor) {
                throw BindException(utils::String::Format(
                    "Type definition '%s' cannot be finalized: It is not trivially destructible and has no destructor bound",
                    m_type->m_name.c_str()
                ));
            }

            m_isFinal = true;
            if (mod) mod->addHostType(m_type->m_info.host_hash, m_type);
            return m_type;
        }
    };
};