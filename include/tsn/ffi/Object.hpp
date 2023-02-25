#pragma once
#include <tsn/ffi/Object.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataType.h>
#include <tsn/bind/bind.h>
#include <tsn/bind/calling.hpp>
#include <tsn/utils/function_match.hpp>

#include <utils/Array.hpp>
#include <type_traits>

namespace tsn {
    template <typename... Args>
    Object::Object(Context* ctx, ffi::DataType* tp, Args&&... args) : ITypedObject(tp), IContextual(ctx) {
        ffi::Function* ctor = nullptr;

        constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
        const ffi::DataType* argTps[argc + 1] = { m_ctx->getTypes()->getType<Args>(std::forward<Args>(args))..., nullptr };

        utils::Array<ffi::Function*> ctors = function_match(
            tp->getName() + "::constructor",
            nullptr,
            argTps,
            argc,
            tp->getMethods(),
            fm_skip_implicit_args | fm_strict
        );

        if (ctors.size() == 1) {
            ctor = ctors[0];
        } else if (ctors.size() > 1) {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has more than one constructor with the provided argument types. Call is ambiguous",
                m_type->getName().c_str()
            ));
        } else {
            if constexpr (std::tuple_size_v<std::tuple<Args...>> == 0) {
                if (!tp->getInfo().is_trivially_constructible) {
                    throw ffi::BindException(utils::String::Format(
                        "Object of type '%s' has no default constructor and is not trivially constructible",
                        m_type->getName().c_str()
                    ));
                }
            } else {
                throw ffi::BindException(utils::String::Format(
                    "Object of type '%s' has no constructor that matches the provided argument types",
                    m_type->getName().c_str()
                ));
            }
        }

        if (tp->getInfo().size > 0) {
            m_data = utils::Mem::alloc(tp->getInfo().size);
            m_dataRefCount = new u32(1);
        } else {
            m_data = nullptr;
            m_dataRefCount = nullptr;
        }

        if (ctor) ffi::call_method(m_ctx, ctor, m_data, args...);
    }

    template <typename T>
    Object Object::prop(const utils::String& propName, const T& value) {
        const auto& props = m_type->getProperties();
        i64 idx = props.findIndex([&propName](const ffi::type_property& p) { return p.name == propName; });
        if (idx < 0) {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has no property named '%s' bound",
                m_type->getName().c_str(), propName.c_str()
            ));
        }

        const auto& p = props[(u32)idx];

        if (p.access == private_access) {
            throw ffi::BindException(utils::String::Format(
                "Cannot set property '%s' of object of type '%s'. The property is private",
                propName.c_str(), m_type->getName().c_str()
            ));
        }

        if (!p.flags.can_write) {
            throw ffi::BindException(utils::String::Format(
                "Cannot set property '%s' of object of type '%s'. The property is not writeable",
                propName.c_str(), m_type->getName().c_str()
            ));
        }

        if (p.setter) {
            if (p.flags.is_static) {
                return ffi::call(m_ctx, p.setter, value);
            } else {
                return ffi::call_method(m_ctx, p.setter, m_data, value);
            }
        }

        T* ptr = (T*)((u8*)m_data + p.offset);
        *ptr = value;
        return Object(m_ctx, false, p.type, (void*)ptr);
    }

    template <typename... Args>
    Object Object::call(const utils::String& funcName, Args&&... args) {
        constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
        const ffi::DataType* argTps[argc + 1] = { m_ctx->getTypes()->getType<Args>(std::forward<Args>(args))..., nullptr };

        utils::Array<ffi::Function*> matches = function_match(
            m_type->getName() + "::" + funcName,
            nullptr,
            argTps,
            argc,
            m_type->getMethods(),
            fm_skip_implicit_args | fm_strict
        );

        ffi::Function* f = nullptr;

        if (matches.size() == 1) {
            f = matches[0];
        } else if (matches.size() > 1) {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has more than one method named '%s' with the provided argument types. Call is ambiguous",
                m_type->getName().c_str(), funcName.c_str()
            ));
        } else {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has no method named '%s'",
                m_type->getName().c_str(), funcName.c_str()
            ));
        }

        if (f->isThisCall()) return ffi::call_method(m_ctx, f, m_data, std::forward<Args>(args)...);
        else return ffi::call(m_ctx, f, std::forward<Args>(args)...);
    }

    template <typename T>
    Object::operator T&() {
        return *(T*)m_data;
    }
    
    template <typename T>
    Object::operator const T&() const {
        return *(T*)m_data;
    }
    
    template <typename T>
    Object::operator T*() {
        return (T*)m_data;
    }

    template <typename T>
    Object::operator const T*() const {
        return (T*)m_data;
    }
};