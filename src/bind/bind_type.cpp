#include <tsn/bind/bind_type.h>
#include <tsn/bind/bind.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/common/Module.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        DataTypeBinder::DataTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName, type_meta&& meta) {
            funcRegistry = freg;
            typeRegistry = treg;
            m_mod = mod;

            DataType* existing = treg->getTypeByHostHash(meta.host_hash);
            if (existing) {
                throw BindException(utils::String::Format(
                    "Attempted to bind new type '%s' which has already been bound (as '%s')",
                    name.c_str(), existing->getFullyQualifiedName().c_str()
                ));
            }

            m_type = new DataType(name, fullyQualifiedName, meta);
            m_type->m_id = (type_id)std::hash<utils::String>()(m_type->getFullyQualifiedName());
            typeRegistry->addHostType(meta.host_hash, m_type);

            if (mod) mod->addHostType(m_type->m_info.host_hash, m_type);
            else typeRegistry->getContext()->getGlobal()->addHostType(m_type->m_info.host_hash, m_type);
            m_isFinal = false;
        }

        DataTypeBinder::~DataTypeBinder() {
            if (!m_isFinal && m_type) {
                typeRegistry->removeType(m_type);
                delete m_type;
            }
            m_type = nullptr;
        }
        
        type_meta& DataTypeBinder::info() {
            return m_type->m_info;
        }
        
        void DataTypeBinder::setAccessModifier(access_modifier access) {
            m_type->m_access = access;
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
            return m_type->m_properties.some([name](const type_property& p) {
                return p.name == name;
            });
        }

        DataType* DataTypeBinder::finalize() {
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

            if (m_type->m_destructor) {
                if (m_mod) m_mod->addFunction(m_type->m_destructor);
            }

            for (u32 i = 0;i < m_type->m_methods.size();i++) {
                if (m_mod) m_mod->addFunction(m_type->m_methods[i]);
            }

            return m_type;
        }


        
        DataTypeExtender::DataTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, type_meta&& meta) {
            funcRegistry = freg;
            typeRegistry = treg;

            m_type = treg->getTypeByHostHash(meta.host_hash);
            m_mod = mod;
            if (!m_type) {
                throw BindException("Attempted to extend type which has not been bound");
            }
        }

        DataTypeExtender::~DataTypeExtender() {
            m_type = nullptr;
        }
        
        type_meta& DataTypeExtender::info() {
            return m_type->m_info;
        }

        void DataTypeExtender::addMethod(Function* method) {
            m_type->m_methods.push(method);
        }

        void DataTypeExtender::addProperty(type_property&& prop) {
            m_type->m_properties.push(prop);
        }
        
        bool DataTypeExtender::propNameExists(const utils::String& name) const {
            return m_type->m_properties.some([name](const type_property& p) {
                return p.name == name;
            });
        }
    };
};