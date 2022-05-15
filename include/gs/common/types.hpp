#pragma once
#include <gs/common/types.hpp>

#include <type_traits>
#include <typeinfo>
#include <typeindex>

namespace gs {
    template <typename T>
    type_meta meta() {
        u16 sz = 0;
        if constexpr (!std::is_same_v<void, T>) sz = (u16)sizeof(T);
        return {
            std::is_trivial_v<T> && std::is_standard_layout_v<T>,
            std::is_trivially_constructible_v<T>,
            std::is_trivially_copyable_v<T>,
            std::is_trivially_destructible_v<T>,
            std::is_fundamental_v<T>,
            std::is_floating_point_v<T>,
            std::is_integral_v<T>,
            std::is_unsigned_v<T>,
            std::is_function_v<T>,
            true,
            sz,
            std::type_index(typeid(T)).hash_code()
        };
    }

    template <typename T>
    std::size_t type_hash() {
        return std::type_index(typeid(T)).hash_code();
    }

    template <typename T>
    const char* type_name() {
        if constexpr (std::is_same_v<void, T>) return "void";
        else return std::type_index(typeid(remove_all<T>::type)).name();
    }
};