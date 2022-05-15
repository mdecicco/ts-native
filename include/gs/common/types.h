#pragma once
#include <stdint.h>

namespace gs {
    typedef uint64_t    u64;
    typedef int64_t     i64;
    typedef uint32_t    u32;
    typedef int32_t     i32;
    typedef uint16_t    u16;
    typedef int16_t     i16;
    typedef uint8_t     u8;
    typedef int8_t      i8;
    typedef float       f32;
    typedef double      f64;

    typedef u64         type_id;
    typedef u64         function_id;

    enum access_modifier {
        public_access,
        private_access
    };

    enum class arg_type {
        func_ptr,
        ret_ptr,
        context_ptr,
        this_ptr,
        value,
        pointer
    };

    struct type_meta {
        unsigned is_pod : 1;
        unsigned is_trivially_constructible : 1;
        unsigned is_trivially_copyable : 1;
        unsigned is_trivially_destructible : 1;
        unsigned is_primitive : 1;
        unsigned is_floating_point : 1;
        unsigned is_integral : 1;
        unsigned is_unsigned : 1;
        unsigned is_function : 1;
        unsigned is_host : 1;
        unsigned size : 16;
        size_t host_hash;
    };

    enum value_flag_mask {
        vf_read    = 0b0001,
        vf_write   = 0b0010,
        vf_static  = 0b0100,
        vf_pointer = 0b1000,
        vf_rw      = 0b0011
    };

    struct value_flags {
        unsigned can_read : 1;
        unsigned can_write : 1;
        unsigned is_static : 1;
        unsigned is_pointer : 1;
    };

    template <typename T>
    type_meta meta();

    template <typename T>
    size_t type_hash();

    template <typename T>
    const char* type_name();
};