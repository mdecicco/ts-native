#pragma once
#include <gs/common/Object.h>
#include <gs/common/DataType.h>
#include <gs/utils/function_match.hpp>

#include <utils/Array.hpp>

namespace gs {
    template <typename... Args>
    Object::Object(ffi::DataTypeRegistry* reg, ffi::DataType* tp, Args... args) : ITypedObject(tp) {
        ffi::Function* ctor = nullptr;

        utils::Array<ffi::Function*> ctors = function_match<void, Args...>(
            reg,
            tp->getName() + "::constructor",
            tp->getMethods(),
            fm_strict | fm_skip_implicit_args
        );

        if (ctors.size() == 1) {
            ctor = ctors[0];
        } else if (ctors.size() > 1) {
            // todo exception
        } else {
            if constexpr (std::tuple_size_v<std::tuple<Args...>> == 0) {
                if (!tp->getInfo().is_trivially_constructible) {
                    // todo exception
                }
            } else {
                // todo exception
            }
        }

        if (tp->getInfo().size > 0) {
            m_data = utils::Mem::alloc(tp->getInfo().size);
            m_dataRefCount = new u32(1);
        } else {
            m_data = nullptr;
            m_dataRefCount = nullptr;
        }

        if (ctor) call_method(ctor, m_data, args...);
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