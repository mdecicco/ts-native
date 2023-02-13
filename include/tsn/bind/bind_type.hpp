#pragma once
#include <tsn/bind/bind_type.h>
#include <tsn/bind/bind.h>
#include <tsn/common/DataType.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>

#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        //
        // PrimitiveTypeBinder
        //

        template <typename Cls>
        PrimitiveTypeBinder<Cls>::PrimitiveTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName)
        : DataTypeBinder(mod, freg, treg, name, fullyQualifiedName, meta<Cls>()) { }

        template <typename Cls>
        PrimitiveTypeBinder<Cls>::~PrimitiveTypeBinder() {
        }

        template <typename Cls>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::access(access_modifier access) {
            setAccessModifier(access);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access) {
            Function* f = bind_pseudo_method<Cls, Ret, Args...>(m_mod, funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        PrimitiveTypeBinder<Cls>& PrimitiveTypeBinder<Cls>::staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access) {
            Function* f = bind_function(m_mod, funcRegistry, typeRegistry, name, method, access, m_type);
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
                getter ? bind_function(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access, m_type) : nullptr,
                setter ? bind_function(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access, m_type) : nullptr
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
                getter ? bind_function(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access, m_type) : nullptr,
                setter ? bind_function(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access, m_type) : nullptr
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
                getter ? bind_pseudo_function<Cls, T&>(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access) : nullptr,
                setter ? bind_pseudo_function<Cls, T&, const T&>(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access) : nullptr
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
                getter ? bind_pseudo_function<Cls, T&>(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access) : nullptr,
                setter ? bind_pseudo_function<Cls, T&, T>(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access) : nullptr
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
        ObjectTypeBinder<Cls>::ObjectTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName)
        : DataTypeBinder(mod, freg, treg, name, fullyQualifiedName, meta<Cls>()) {
        }

        template <typename Cls>
        ObjectTypeBinder<Cls>::~ObjectTypeBinder() {
        }

        template <typename Cls>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::access(access_modifier access) {
            setAccessModifier(access);
            return *this;
        }

        template <typename Cls>
        template <typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::ctor(access_modifier access) {
            Function* f = bind_constructor<Cls, Args...>(m_mod, funcRegistry, typeRegistry, m_type, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::dtor(access_modifier access) {
            Function* f = bind_destructor<Cls>(m_mod, funcRegistry, typeRegistry, m_type, access);
            if (f) setDestructor(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (Cls::*method)(Args...), access_modifier access) {
            Function* f = bind_method(m_mod, funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access) {
            Function* f = bind_method(m_mod, funcRegistry, typeRegistry, name, method, access);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename Ret, typename... Args>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access) {
            Function* f = bind_function(m_mod, funcRegistry, typeRegistry, name, method, access, m_type);
            if (f) addMethod(f);
            return *this;
        }

        template <typename Cls>
        template <typename T>
        ObjectTypeBinder<Cls>& ObjectTypeBinder<Cls>::prop(const utils::String& name, T Cls::* member, access_modifier access, value_flag_mask flags) {
            if (!checkNewProp<T>(name)) return *this;

            DataType* tp = typeRegistry->getType<T>();
            if (std::is_pointer_v<T> || std::is_reference_v<T>) flags |= vf_pointer;
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
            if (std::is_pointer_v<T> || std::is_reference_v<T>) flags |= vf_pointer;
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
                getter ? bind_function(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access, m_type) : nullptr,
                setter ? bind_function(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access, m_type) : nullptr
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
                getter ? bind_function(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access, m_type) : nullptr,
                setter ? bind_function(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access, m_type) : nullptr
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
                getter ? bind_method(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access) : nullptr,
                setter ? bind_method(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access) : nullptr
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
                getter ? bind_method(m_mod, funcRegistry, typeRegistry, "$get_" + name, getter, private_access) : nullptr,
                setter ? bind_method(m_mod, funcRegistry, typeRegistry, "$set_" + name, setter, private_access) : nullptr
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