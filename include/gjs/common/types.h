#pragma once
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define FORCE_INLINE __forceinline
#elif __APPLE__
    #define FORCE_INLINE
#elif __linux__
    #define FORCE_INLINE
#else
    #define FORCE_INLINE
#endif

namespace gjs {
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

    typedef u64         address;

    // 0 = invalid
    typedef u64         function_id;

    // 0 = invalid
    typedef u32         label_id;
};
