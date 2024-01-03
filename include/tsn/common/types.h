#pragma once
#include <stdint.h>

namespace tsn {
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

    typedef u32         type_id;
    typedef u32         function_id;
    
    typedef struct { u8 dummy; }    poison_t;
    typedef struct { u64 dummy; }   null_t;

    namespace ffi {
        class ExecutionContext;
    };

    struct call_context {
        ffi::ExecutionContext* ectx;
        void* funcPtr;
        void* retPtr;
        void* thisPtr;
        void* capturePtr;
    };

    enum access_modifier {
        /** Accessible to everyone */
        public_access,

        /** Accessible only to owner */
        private_access,

        /** Accessible only to trusted scripts */
        trusted_access
    };

    enum class arg_type {
        context_ptr,
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
        unsigned is_template : 1;
        unsigned is_alias : 1;
        unsigned is_host : 1;
        unsigned is_anonymous : 1;
        unsigned size : 16;
        size_t host_hash;
    };

    enum _value_flag_mask {
        vf_read    = 0b00000001,
        vf_write   = 0b00000010,
        vf_static  = 0b00000100,
        vf_pointer = 0b00001000,
        vf_rw      = 0b00000011
    };
    typedef u8 value_flag_mask;

    struct value_flags {
        unsigned can_read : 1;
        unsigned can_write : 1;
        unsigned is_static : 1;
        unsigned is_pointer : 1;
    };

    /**
     * @brief Function match search options
     */
    enum _function_match_flags {
        /**
         * @brief Ignore the implicit arguments of functions being compared to the provided signature
         */
        fm_skip_implicit_args = 0b00000001,

        /**
         * @brief Ignore all argument related checks
         */
        fm_ignore_args        = 0b00000010,
        
        /**
         * @brief Exclude functions with signatures that don't strictly match the provided return type.
         *        Without this flag, return types that are convertible to the provided return type with
         *        only one degree of separation will be considered matching.
         */
        fm_strict_return      = 0b00000100,
        
        /**
         * @brief Exclude functions with signatures that don't strictly match the provided argument types.
         *        Without this flag, argument types that are convertible to the provided argument types
         *        with only one degree of separation will be considered matching.
         */
        fm_strict_args        = 0b00001000,
        
        /**
         * @brief Same as fm_strict_return | fm_strict_args
         */
        fm_strict             = fm_strict_return | fm_strict_args,

        /**
         * @brief Exclude private functions
         */
        fm_exclude_private    = 0b00010000
    };
    typedef u8 function_match_flags;

    //
    // Utility functions
    //

    template <typename T>
    type_meta meta();

    template <typename T>
    size_t type_hash();

    template <typename T>
    const char* type_name();

    value_flags convertPropertyMask(value_flag_mask mask);
};