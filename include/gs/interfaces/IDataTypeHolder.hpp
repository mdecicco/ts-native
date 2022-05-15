#pragma once
#include <gs/interfaces/IDataTypeHolder.h>
#include <gs/common/DataType.h>
#include <gs/common/types.hpp>

namespace gs {
    template <typename T>
    ffi::DataType* IDataTypeHolder::getType() const {
        auto it = m_typeHashMap.find(type_hash<T>());
        if (it == m_typeHashMap.end()) return nullptr;
        return m_types[it->second];
    }
};