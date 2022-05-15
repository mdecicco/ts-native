#pragma once
#include <gs/interfaces/IDataTypeHolder.h>
#include <gs/bind/bind.h>
#include <gs/common/DataType.h>
#include <gs/common/Object.h>
#include <gs/common/types.hpp>
#include <gs/utils/remove_all.h>

namespace gs {
    template <typename T>
    ffi::DataType* IDataTypeHolder::getType() const {
        auto it = m_typeHashMap.find(type_hash<ffi::remove_all_except_void_ptr<T>::type>());
        if (it == m_typeHashMap.end()) return nullptr;
        return m_types[it->second];
    }
            
    template <typename T>
    ffi::DataType* IDataTypeHolder::getType(T&& arg) const {
        if constexpr (std::is_same_v<Object, ffi::remove_all<T>::type>) {
            Object* o = nullptr;
            if constexpr (std::is_pointer_v<T>) o = arg;
            else if constexpr (std::is_reference_v<T>) {
                if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) o = arg;
                else o = &arg;
            } else if constexpr (std::is_same_v<Object, T>) o = &arg;
            else {
                throw ffi::BindException("IDataTypeHolder::getType(T&& arg): Failed to get type. T is some form of 'gs::Object' that is unsupported");
            }

            return o->getType();
        } else {
            auto it = m_typeHashMap.find(type_hash<ffi::remove_all_except_void_ptr<T>::type>());
            if (it == m_typeHashMap.end()) return nullptr;
            return m_types[it->second];
        }
    }
};