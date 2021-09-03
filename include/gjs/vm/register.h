#pragma once
#include <gjs/common/types.h>

namespace gjs {
    // number of bits needed to store register id: 6
    // max number of registers: 63
    enum class vm_register {
        zero = 0,    // always zero, read only

        // stores results
        v0        ,
        v1        ,
        v2        ,
        v3        ,
        vf0       ,
        vf1       ,
        vf2       ,
        vf3       ,

        // function arguments
        a0        ,
        a1        ,
        a2        ,
        a3        ,
        a4        ,
        a5        ,
        a6        ,
        a7        ,
        fa0       ,
        fa1       ,
        fa2       ,
        fa3       ,
        fa4       ,
        fa5       ,
        fa6       ,
        fa7       ,

        // temporary storage
        s0        ,
        s1        ,
        s2        ,
        s3        ,
        s4        ,
        s5        ,
        s6        ,
        s7        ,
        s8        ,
        s9        ,
        s10       ,
        s11       ,
        s12       ,
        s13       ,
        s14       ,
        s15       ,

        // floating point storage
        f0        ,
        f1        ,
        f2        ,
        f3        ,
        f4        ,
        f5        ,
        f6        ,
        f7        ,
        f8        ,
        f9        ,
        f10       ,
        f11       ,
        f12       ,
        f13       ,
        f14       ,
        f15       ,

        // execution context
        ip        ,    // instruction pointer
        ra        ,    // return address
        sp        ,    // stack pointer

        register_count    // number of registers
    };

    #define vm_reg_is_volatile(r) (r >= vm_register::v0 && r <= vm_register::vf3)
    #define vm_reg_is_arg(r) (r >= vm_register::a0 && r <= vm_register::fa7)
    #define is_fpr(x) ((x >= vmr::f0 && x <= vmr::f15) || (x >= vmr::fa0 && x <= vmr::fa7) || (x >= vmr::vf0 && x <= vmr::vf3))

    extern const char* register_str[(i32)vm_register::register_count];
};