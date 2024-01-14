#pragma once
#include <tsn/common/types.hpp>
#include <tsn/utils/remove_all.h>

#include <type_traits>
#include <typeinfo>
#include <typeindex>

namespace tsn {
    template <typename T>
    type_meta meta() {
        u16 sz = 0;
        if constexpr (!std::is_same_v<void, T>) sz = (u16)sizeof(T);
        return {
            std::is_trivial_v<T> && std::is_standard_layout_v<T>, // is_pod
            std::is_trivially_constructible_v<T>,                 // is_trivially_constructible
            std::is_trivially_copyable_v<T>,                      // is_trivially_copyable
            std::is_trivially_destructible_v<T>,                  // is_trivially_destructible
            std::is_fundamental_v<T>,                             // is_primitive
            std::is_floating_point_v<T>,                          // is_floating_point
            std::is_integral_v<T>,                                // is_integral
            std::is_unsigned_v<T>,                                // is_unsigned
            std::is_function_v<T>,                                // is_function
            0,                                                    // is_template
            0,                                                    // is_alias
            1,                                                    // is_host
            0,                                                    // is_anonymous
            sz,                                                   // size
            std::type_index(typeid(T)).hash_code()                // host_hash
        };
    }

    template <typename T>
    std::size_t type_hash() {
        return std::type_index(typeid(T)).hash_code();
    }

    template <typename T>
    const char* type_name() {
        if constexpr (std::is_same_v<void, T>) return "void";
        else return std::type_index(typeid(typename ffi::remove_all<T>::type)).name();
    }
};