#pragma once
#include <gs/bind/bind_type.h>
#include <gs/bind/bind.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>

#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        //
        // Utility functions
        //

        value_flags convertPropertyMask(value_flag_mask mask) {
            return {
                mask & (u32)vf_read,
                mask & (u32)vf_write,
                mask & (u32)vf_static,
                mask & (u32)vf_pointer
            };
        }



        //
        // PrimitiveTypeBinder
        //

        template <typename Cls>
        PrimitiveTypeBinder<Cls>::PrimitiveTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName)
        : DataTypeBinder(freg, treg, name, fullyQualifiedName, meta<Cls>()) { }

        template <typename Cls>
        PrimitiveTypeBinder<Cls>::~PrimitiveTypeBinder() {
        }

        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T* member, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            addProperty({
                name,
                // Hard coded because if it's private then just don't bind it...
                public_access,
                (u64)member,
                typeRegistry->getType<T>(),
                convertPropertyMask(flags)
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        bool PrimitiveTypeBinder<Cls>::checkNewProp(const utils::String& name) const {
            if (!typeRegistry->getType<T>()) {
                throw BindException(utils::String::Format(
                    "Cannot bind property '%s' of type '%s' to type '%s', type '%s' has not been bound",
                    name.c_str(), type_name<T>(), m_type->getName().c_str(), type_name<T>()
                ));
                
                return false;
            }

            bool exists = propNameExists(name);

            if (exists) {
                throw BindException(utils::String::Format(
                    "Type '%s' already has a property named '%s'",
                    m_type->getName().c_str(), name.c_str()
                ));
            }

            return exists;
        }



        //
        // ObjectTypeBinder
        //

        template <typename Cls>
        ObjectTypeBinder<Cls>::ObjectTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName)
        : DataTypeBinder(freg, treg, name, fullyQualifiedName, meta<Cls>()) {
        }

        template <typename Cls>
        ObjectTypeBinder<Cls>::~ObjectTypeBinder() {
        }

        template <typename Cls>
        template <typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::ctor(access_modifier access) {
            Function* f = bind_constructor<Cls, Args...>(funcRegistry, typeRegistry, m_type, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::dtor(access_modifier access) {
            Function* f = bind_destructor<Cls>(funcRegistry, typeRegistry, m_type, access);
            if (f) setDestructor(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (Cls::*method)(Args...), access_modifier access) {
            Function* f = bind_method(funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access) {
            Function* f = bind_method(funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (*method)(Args...), access_modifier access) {
            Function* f = bind_function(funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T Cls::* member, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            u64 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
            addProperty({
                name,
                // Hard coded because if it's private then just don't bind it...
                public_access,
                offset,
                typeRegistry->getType<T>(),
                convertPropertyMask(flags)
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T* member, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            addProperty({
                name,
                // Hard coded because if it's private then just don't bind it...
                public_access,
                (u64)member,
                typeRegistry->getType<T>(),
                convertPropertyMask(flags)
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        bool ObjectTypeBinder<Cls>::checkNewProp(const utils::String& name) const {
            if (!typeRegistry->getType<T>()) {
                throw BindException(utils::String::Format(
                    "Cannot bind property '%s' of type '%s' to type '%s', type '%s' has not been bound",
                    name.c_str(), type_name<T>(), m_type->getName().c_str(), type_name<T>()
                ));
                
                return false;
            }

            bool exists = propNameExists(name);

            if (exists) {
                throw BindException(utils::String::Format(
                    "Type '%s' already has a property named '%s'",
                    m_type->getName().c_str(), name.c_str()
                ));
            }

            return exists;
        }
    };
};