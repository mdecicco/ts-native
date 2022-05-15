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
                (u32)((mask & (u32)vf_read) ? 1 : 0),
                (u32)((mask & (u32)vf_write) ? 1 : 0),
                (u32)((mask & (u32)vf_static) ? 1 : 0),
                (u32)((mask & (u32)vf_pointer) ? 1 : 0)
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
        template <typename Ret, typename... Args>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access) {
            Function* f = bind_pseudo_method<Cls, Ret, Args...>(funcRegistry, typeRegistry, m_type->getName() + "::" + name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access) {
            Function* f = bind_function(funcRegistry, typeRegistry, m_type->getName() + "::" + name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T* member, access_modifier access, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            DataType* tp = typeRegistry->getType<T>();
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;
            flags |= vf_static;

            addProperty({
                name,
                access,
                (u64)member,
                tp,
                convertPropertyMask(flags),
                nullptr,
                nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = vf_static;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }
        
        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = vf_static;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, const T&), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = 0;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_pseudo_function<Cls, T&>(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_pseudo_function<Cls, T&, const T&>(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, T), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = 0;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_pseudo_function<Cls, T&>(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_pseudo_function<Cls, T&, T>(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
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

            return !exists;
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
            Function* f = bind_method(funcRegistry, typeRegistry, m_type->getName() + "::" + name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access) {
            Function* f = bind_method(funcRegistry, typeRegistry, m_type->getName() + "::" + name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access) {
            Function* f = bind_function(funcRegistry, typeRegistry, m_type->getName() + "::" + name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T Cls::* member, access_modifier access, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            DataType* tp = typeRegistry->getType<T>();
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;
            flags &= ~vf_static;

            u64 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
            addProperty({
                name,
                access,
                offset,
                tp,
                convertPropertyMask(flags),
                nullptr,
                nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T* member, access_modifier access, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            DataType* tp = typeRegistry->getType<T>();
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;
            flags |= vf_static;

            addProperty({
                name,
                access,
                (u64)member,
                tp,
                convertPropertyMask(flags),
                nullptr,
                nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = vf_static;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = vf_static;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_function(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T (Cls::*getter)(Cls*), T (Cls::*setter)(const T&), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = 0;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_method(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_method(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
            });

            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T (Cls::*getter)(Cls*), T (Cls::*setter)(T), access_modifier access) {
            if (!checkNewProp<T>(name)) return *this;

            value_flag_mask flags = 0;

            DataType* tp = typeRegistry->getType<T>();
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;
            if (!tp->getInfo().is_primitive) flags |= vf_pointer;

            addProperty({
                name,
                access,
                0,
                tp,
                convertPropertyMask(flags),
                getter ? bind_method(funcRegistry, typeRegistry, m_type->getName() + "::$get_" + name, getter, private_access) : nullptr,
                setter ? bind_method(funcRegistry, typeRegistry, m_type->getName() + "::$set_" + name, setter, private_access) : nullptr
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

            return !exists;
        }
    };
};