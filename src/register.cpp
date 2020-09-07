#include <register.h>

namespace gjs {
    extern const char* register_str[(i32)vm_register::register_count] = {
        "zero",

        // stores results
        "v0",
        "v1",
        "v2",
        "v3",

        // function arguments
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",

        // temporary storage
        "s0",
        "s1",
        "s2",
        "s3",
        "s4",
        "s5",
        "s6",
        "s7",

        // floating point
        "f0",
        "f1",
        "f2",
        "f3",
        "f4",
        "f5",
        "f6",
        "f7",
        "f8",
        "f9",
        "f10",
        "f11",
        "f12",
        "f13",
        "f14",
        "f15",

        // execution context
        "ip",
        "ra",
        "sp"
    };
};