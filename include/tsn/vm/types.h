#pragma once
#include <tsn/common/types.h>

namespace tsn {
    namespace vm {
        typedef u32 address;
        
        // number of bits needed to store register id: 7
        // max number of registers: 127
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
            a8        ,
            a9        ,
            a10       ,
            a11       ,
            a12       ,
            a13       ,
            a14       ,
            a15       ,
            fa0       ,
            fa1       ,
            fa2       ,
            fa3       ,
            fa4       ,
            fa5       ,
            fa6       ,
            fa7       ,
            fa8       ,
            fa9       ,
            fa10      ,
            fa11      ,
            fa12      ,
            fa13      ,
            fa14      ,
            fa15      ,

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

            register_count // number of registers
        };

        // number of bits needed to store instruction id: 9
        // max number of instructions 512
        enum class vm_instruction {
            null         = 0,    // do nothing
            term            ,    // terminate execution

            // load src/store dest can have an offset, and load from/store in the memory pointed to
            // by (src) + offset / (dest) + offset.

            // floating point registers can only be used by the floating point arithmetic instructions,
            // the instructions that move data to/from them, and the instructions that convert values
            // in them between integer and floating point before/after moving from/to a gp register

            // memory moving
            ld8             ,    // load 1 byte from memory into register               ld8       (dest)   (src)       i_off  dest = *((u8*)(src + i_off))
            ld16            ,    // load 2 bytes from memory into register              ld16      (dest)   (src)       i_off  dest = *((u16*)(src + i_off))
            ld32            ,    // load 4 bytes from memory into register              ld32      (dest)   (src)       i_off  dest = *((u32*)(src + i_off))
            ld64            ,    // load 8 bytes from memory into register              ld64      (dest)   (src)       i_off  dest = *((u64*)(src + i_off))
            st8             ,    // store 1 byte in memory from register                store     (src)    (dest)      i_off  *((u8*)(dest + i_off)) = src
            st16            ,    // store 2 bytes in memory from register               store     (src)    (dest)      i_off  *((u16*)(dest + i_off)) = src
            st32            ,    // store 4 bytes in memory from register               store     (src)    (dest)      i_off  *((u32*)(dest + i_off)) = src
            st64            ,    // store 8 bytes in memory from register               store     (src)    (dest)      i_off  *((u64*)(dest + i_off)) = src
            mptr            ,    // store address of module data in register            mptr      (dest)               i_off  dest = modules[$v3] + 0xf    ($v3 must be module id)
            mtfp            ,    // move from general register to float register        mtfp      (a)      (b)                b = a                        (a must be gp, b must be fp)
            mffp            ,    // move from float register to general register        mffp      (a)      (b)                b = a                        (a must be fp, b must be gp)
            
            // vector operations
            // (vector arguments must be pointers to vectors)
            v2fset          ,    // set 2D vector (f32) from another                    v2fset    (a)      (b)                a.x = b, a.y = b
            v2fsets         ,    // set 2D vector (f32) from scalar                     v2fsets   (a)      (b)                a.x = b, a.y = b
            v2fsetsi        ,    // set 2D vector (f32) from scalar immediate           v2fsetsi  (a)      (b imm)            a.x = b, a.y = b
            v2dset          ,    // set 2D vector (f64) from another                    v2dset    (a)      (b)                a.x = b, a.y = b
            v2dsets         ,    // set 2D vector (f64) from scalar                     v2dsets   (a)      (b)                a.x = b, a.y = b
            v2dsetsi        ,    // set 2D vector (f64) from scalar immediate           v2dsetsi  (a)      (b imm)            a.x = b, a.y = b
            v2fadd          ,    // add 2D vector (f32) to another                      v2fadd    (a)      (b)                a.x += b.x, a.y += b.y
            v2fadds         ,    // add scalar to 2D vector (f32)                       v2fadds   (a)      (b)                a.x += b, a.y += b
            v2faddsi        ,    // add scalar immediate to 2D vector (f32)             v2faddsi  (a)      (b imm)            a.x += b, a.y += b
            v2dadd          ,    // add 2D vector (f64) to another                      v2dadd    (a)      (b)                a.x += b.x, a.y += b.y
            v2dadds         ,    // add scalar to 2D vector (f64)                       v2dadds   (a)      (b)                a.x += b, a.y += b
            v2daddsi        ,    // add scalar immediate to 2D vector (f64)             v2daddsi  (a)      (b imm)            a.x += b, a.y += b
            v2fsub          ,    // subtract 2D vector (f32) from another               v2fsub    (a)      (b)                a.x -= b.x, a.y -= b.y
            v2fsubs         ,    // subtract scalar from 2D vector (f32)                v2fsubs   (a)      (b)                a.x -= b, a.y -= b
            v2fsubsi        ,    // subtract scalar immediate from 2D vector (f32)      v2fsubsi  (a)      (b imm)            a.x -= b, a.y -= b
            v2dsub          ,    // subtract 2D vector (f64) from another               v2dsub    (a)      (b)                a.x -= b.x, a.y -= b.y
            v2dsubs         ,    // subtract scalar from 2D vector (f64)                v2dsubs   (a)      (b)                a.x -= b, a.y -= b
            v2dsubsi        ,    // subtract scalar immediate from 2D vector (f64)      v2dsubsi  (a)      (b imm)            a.x -= b, a.y -= b
            v2fmul          ,    // multiply 2D vector (f32) by another                 v2fmul    (a)      (b)                a.x *= b.x, a.y *= b.y
            v2fmuls         ,    // multiply 2D vector (f32) by scalar                  v2fmuls   (a)      (b)                a.x *= b, a.y *= b
            v2fmulsi        ,    // multiply 2D vector (f32) by scalar immediate        v2fmulsi  (a)      (b imm)            a.x *= b, a.y *= b
            v2dmul          ,    // multiply 2D vector (f64) by another                 v2dmul    (a)      (b)                a.x *= b.x, a.y *= b.y
            v2dmuls         ,    // multiply 2D vector (f64) by scalar                  v2dmuls   (a)      (b)                a.x *= b, a.y *= b
            v2dmulsi        ,    // multiply 2D vector (f64) by scalar immediate        v2dmulsi  (a)      (b imm)            a.x *= b, a.y *= b
            v2fdiv          ,    // divide 2D vector (f32) by another                   v2fdiv    (a)      (b)                a.x /= b.x, a.y /= b.y
            v2fdivs         ,    // divide 2D vector (f32) by scalar                    v2fdivs   (a)      (b)                a.x /= b, a.y /= b
            v2fdivsi        ,    // divide 2D vector (f32) by scalar immediate          v2fdivsi  (a)      (b imm)            a.x /= b, a.y /= b
            v2ddiv          ,    // divide 2D vector (f64) by another                   v2ddiv    (a)      (b)                a.x /= b.x, a.y /= b.y
            v2ddivs         ,    // divide 2D vector (f64) by scalar                    v2ddivs   (a)      (b)                a.x /= b, a.y /= b
            v2ddivsi        ,    // divide 2D vector (f64) by scalar immediate          v2ddivsi  (a)      (b imm)            a.x /= b, a.y /= b
            v2fmod          ,    // modulo 2D vector (f32) by another                   v2fmod    (a)      (b)                a.x %= b.x, a.y %= b.y
            v2fmods         ,    // modulo 2D vector (f32) by scalar                    v2fmods   (a)      (b)                a.x %= b, a.y %= b
            v2fmodsi        ,    // modulo 2D vector (f32) by scalar immediate          v2fmodsi  (a)      (b imm)            a.x %= b, a.y %= b
            v2dmod          ,    // modulo 2D vector (f64) by another                   v2dmod    (a)      (b)                a.x %= b.x, a.y %= b.y
            v2dmods         ,    // modulo 2D vector (f64) by scalar                    v2dmods   (a)      (b)                a.x %= b, a.y %= b
            v2dmodsi        ,    // modulo 2D vector (f64) by scalar immediate          v2dmodsi  (a)      (b imm)            a.x %= b, a.y %= b
            v2fneg          ,    // negate 2D vector (f32)                              v2fneg    (a)                         a.x = -a.x, a.y = -a.y
            v2dneg          ,    // negate 2D vector (f64)                              v2dneg    (a)                         a.x = -a.x, a.y = -a.y
            v2fdot          ,    // get dot product of two 2D vectors (f32)             v2fdot    (a)      (b)        (c)     a = dot(b, c)
            v2ddot          ,    // get dot product of two 2D vectors (f64)             v2ddot    (a)      (b)        (c)     a = dot(b, c)
            v2fmag          ,    // get length of 2D vector (f32)                       v2fmag    (a)      (b)                a = length(b)
            v2dmag          ,    // get length of 2D vector (f64)                       v2dmag    (a)      (b)                a = length(b)
            v2fmagsq        ,    // get squared length of 2D vector (f32)               v2fmagsq  (a)      (b)                a = lengthSq(b)
            v2dmagsq        ,    // get squared length of 2D vector (f64)               v2dmagsq  (a)      (b)                a = lengthSq(b)
            v2fnorm         ,    // normalize 2D vector (f32)                           v2fnorm   (a)                         a /= length(a)
            v2dnorm         ,    // normalize 2D vector (f64)                           v2dnorm   (a)                         a /= length(a)
            
            v3fset          ,    // set 3D vector (f32) from another                    v3fset    (a)      (b)                a.x = b, a.y = b, a.z = b
            v3fsets         ,    // set 3D vector (f32) from scalar                     v3fsets   (a)      (b)                a.x = b, a.y = b, a.z = b
            v3fsetsi        ,    // set 3D vector (f32) from scalar immediate           v3fsetsi  (a)      (b imm)            a.x = b, a.y = b, a.z = b
            v3dset          ,    // set 3D vector (f64) from another                    v3dset    (a)      (b)                a.x = b, a.y = b, a.z = b
            v3dsets         ,    // set 3D vector (f64) from scalar                     v3dsets   (a)      (b)                a.x = b, a.y = b, a.z = b
            v3dsetsi        ,    // set 3D vector (f64) from scalar immediate           v3dsetsi  (a)      (b imm)            a.x = b, a.y = b, a.z = b
            v3fadd          ,    // add 3D vector (f32) to another                      v3fadd    (a)      (b)                a.x += b.x, a.y += b.y, a.z += b.z
            v3fadds         ,    // add scalar to 3D vector (f32)                       v3fadds   (a)      (b)                a.x += b, a.y += b, a.z += b
            v3faddsi        ,    // add scalar immediate to 3D vector (f32)             v3faddsi  (a)      (b imm)            a.x += b, a.y += b, a.z += b
            v3dadd          ,    // add 3D vector (f64) to another                      v3dadd    (a)      (b)                a.x += b.x, a.y += b.y, a.z += b.z
            v3dadds         ,    // add scalar to 3D vector (f64)                       v3dadds   (a)      (b)                a.x += b, a.y += b, a.z += b
            v3daddsi        ,    // add scalar immediate to 3D vector (f64)             v3daddsi  (a)      (b imm)            a.x += b, a.y += b, a.z += b
            v3fsub          ,    // subtract 3D vector (f32) from another               v3fsub    (a)      (b)                a.x -= b.x, a.y -= b.y, a.z -= b.z
            v3fsubs         ,    // subtract scalar from 3D vector (f32)                v3fsubs   (a)      (b)                a.x -= b, a.y -= b, a.z -= b
            v3fsubsi        ,    // subtract scalar immediate from 3D vector (f32)      v3fsubsi  (a)      (b imm)            a.x -= b, a.y -= b, a.z -= b
            v3dsub          ,    // subtract 3D vector (f64) from another               v3dsub    (a)      (b)                a.x -= b.x, a.y -= b.y, a.z -= b.z
            v3dsubs         ,    // subtract scalar from 3D vector (f64)                v3dsubs   (a)      (b)                a.x -= b, a.y -= b, a.z -= b
            v3dsubsi        ,    // subtract scalar immediate from 3D vector (f64)      v3dsubsi  (a)      (b imm)            a.x -= b, a.y -= b, a.z -= b
            v3fmul          ,    // multiply 3D vector (f32) by another                 v3fmul    (a)      (b)                a.x *= b.x, a.y *= b.y, a.z *= b.z
            v3fmuls         ,    // multiply 3D vector (f32) by scalar                  v3fmuls   (a)      (b)                a.x *= b, a.y *= b, a.z *= b
            v3fmulsi        ,    // multiply 3D vector (f32) by scalar immediate        v3fmulsi  (a)      (b imm)            a.x *= b, a.y *= b, a.z *= b
            v3dmul          ,    // multiply 3D vector (f64) by another                 v3dmul    (a)      (b)                a.x *= b.x, a.y *= b.y, a.z *= b.z
            v3dmuls         ,    // multiply 3D vector (f64) by scalar                  v3dmuls   (a)      (b)                a.x *= b, a.y *= b, a.z *= b
            v3dmulsi        ,    // multiply 3D vector (f64) by scalar immediate        v3dmulsi  (a)      (b imm)            a.x *= b, a.y *= b, a.z *= b
            v3fdiv          ,    // divide 3D vector (f32) by another                   v3fdiv    (a)      (b)                a.x /= b.x, a.y /= b.y, a.z /= b.z
            v3fdivs         ,    // divide 3D vector (f32) by scalar                    v3fdivs   (a)      (b)                a.x /= b, a.y /= b, a.z /= b
            v3fdivsi        ,    // divide 3D vector (f32) by scalar immediate          v3fdivsi  (a)      (b imm)            a.x /= b, a.y /= b, a.z /= b
            v3ddiv          ,    // divide 3D vector (f64) by another                   v3ddiv    (a)      (b)                a.x /= b.x, a.y /= b.y, a.z /= b.z
            v3ddivs         ,    // divide 3D vector (f64) by scalar                    v3ddivs   (a)      (b)                a.x /= b, a.y /= b, a.z /= b
            v3ddivsi        ,    // divide 3D vector (f64) by scalar immediate          v3ddivsi  (a)      (b imm)            a.x /= b, a.y /= b, a.z /= b
            v3fmod          ,    // modulo 3D vector (f32) by another                   v3fmod    (a)      (b)                a.x %= b.x, a.y %= b.y, a.z %= b.z
            v3fmods         ,    // modulo 3D vector (f32) by scalar                    v3fmods   (a)      (b)                a.x %= b, a.y %= b, a.z %= b
            v3fmodsi        ,    // modulo 3D vector (f32) by scalar immediate          v3fmodsi  (a)      (b imm)            a.x %= b, a.y %= b, a.z %= b
            v3dmod          ,    // modulo 3D vector (f64) by another                   v3dmod    (a)      (b)                a.x %= b.x, a.y %= b.y, a.z %= b.z
            v3dmods         ,    // modulo 3D vector (f64) by scalar                    v3dmods   (a)      (b)                a.x %= b, a.y %= b, a.z %= b
            v3dmodsi        ,    // modulo 3D vector (f64) by scalar immediate          v3dmodsi  (a)      (b imm)            a.x %= b, a.y %= b, a.z %= b
            v3fneg          ,    // negate 3D vector (f32)                              v3fneg    (a)                         a.x = -a.x, a.y = -a.y, a.z = -a.z
            v3dneg          ,    // negate 3D vector (f64)                              v3dneg    (a)                         a.x = -a.x, a.y = -a.y, a.z = -a.z
            v3fdot          ,    // get dot product of two 3D vectors (f32)             v3fdot    (a)      (b)        (c)     a = dot(b, c)
            v3ddot          ,    // get dot product of two 3D vectors (f64)             v3ddot    (a)      (b)        (c)     a = dot(b, c)
            v3fmag          ,    // get length of 3D vector (f32)                       v3fmag    (a)      (b)                a = length(b)
            v3dmag          ,    // get length of 3D vector (f64)                       v3dmag    (a)      (b)                a = length(b)
            v3fmagsq        ,    // get squared length of 3D vector (f32)               v3fmagsq  (a)      (b)                a = lengthSq(b)
            v3dmagsq        ,    // get squared length of 3D vector (f64)               v3dmagsq  (a)      (b)                a = lengthSq(b)
            v3fnorm         ,    // normalize 3D vector (f32)                           v3fnorm   (a)                         a /= length(a)
            v3dnorm         ,    // normalize 3D vector (f64)                           v3dnorm   (a)                         a /= length(a)
            v3fcross        ,    // get cross product of two 3D vectors (f32)           v3fcross  (a)      (b)        (c)     a = cross(b, c)
            v3dcross        ,    // get cross product of two 3D vectors (f64)           v3dcross  (a)      (b)        (c)     a = cross(b, c)
            
            v4fset          ,    // set 4D vector (f32) from another                    v4fset    (a)      (b)                a.x = b, a.y = b, a.z = b, a.w = b
            v4fsets         ,    // set 4D vector (f32) from scalar                     v4fsets   (a)      (b)                a.x = b, a.y = b, a.z = b, a.w = b
            v4fsetsi        ,    // set 4D vector (f32) from scalar immediate           v4fsetsi  (a)      (b imm)            a.x = b, a.y = b, a.z = b, a.w = b
            v4dset          ,    // set 4D vector (f64) from another                    v4dset    (a)      (b)                a.x = b, a.y = b, a.z = b, a.w = b
            v4dsets         ,    // set 4D vector (f64) from scalar                     v4dsets   (a)      (b)                a.x = b, a.y = b, a.z = b, a.w = b
            v4dsetsi        ,    // set 4D vector (f64) from scalar immediate           v4dsetsi  (a)      (b imm)            a.x = b, a.y = b, a.z = b, a.w = b
            v4fadd          ,    // add 4D vector (f32) to another                      v4fadd    (a)      (b)                a.x += b.x, a.y += b.y, a.z += b.z, a.w += b.w
            v4fadds         ,    // add scalar to 4D vector (f32)                       v4fadds   (a)      (b)                a.x += b, a.y += b, a.z += b, a.w += b
            v4faddsi        ,    // add scalar immediate to 4D vector (f32)             v4faddsi  (a)      (b imm)            a.x += b, a.y += b, a.z += b, a.w += b
            v4dadd          ,    // add 4D vector (f64) to another                      v4dadd    (a)      (b)                a.x += b.x, a.y += b.y, a.z += b.z, a.w += b.w
            v4dadds         ,    // add scalar to 4D vector (f64)                       v4dadds   (a)      (b)                a.x += b, a.y += b, a.z += b, a.w += b
            v4daddsi        ,    // add scalar immediate to 4D vector (f64)             v4daddsi  (a)      (b imm)            a.x += b, a.y += b, a.z += b, a.w += b
            v4fsub          ,    // subtract 4D vector (f32) from another               v4fsub    (a)      (b)                a.x -= b.x, a.y -= b.y, a.z -= b.z, a.w -= b.w
            v4fsubs         ,    // subtract scalar from 4D vector (f32)                v4fsubs   (a)      (b)                a.x -= b, a.y -= b, a.z -= b, a.w -= b
            v4fsubsi        ,    // subtract scalar immediate from 4D vector (f32)      v4fsubsi  (a)      (b imm)            a.x -= b, a.y -= b, a.z -= b, a.w -= b
            v4dsub          ,    // subtract 4D vector (f64) from another               v4dsub    (a)      (b)                a.x -= b.x, a.y -= b.y, a.z -= b.z, a.w -= b.w
            v4dsubs         ,    // subtract scalar from 4D vector (f64)                v4dsubs   (a)      (b)                a.x -= b, a.y -= b, a.z -= b, a.w -= b
            v4dsubsi        ,    // subtract scalar immediate from 4D vector (f64)      v4dsubsi  (a)      (b imm)            a.x -= b, a.y -= b, a.z -= b, a.w -= b
            v4fmul          ,    // multiply 4D vector (f32) by another                 v4fmul    (a)      (b)                a.x *= b.x, a.y *= b.y, a.z *= b.z, a.w *= b.w
            v4fmuls         ,    // multiply 4D vector (f32) by scalar                  v4fmuls   (a)      (b)                a.x *= b, a.y *= b, a.z *= b, a.w *= b
            v4fmulsi        ,    // multiply 4D vector (f32) by scalar immediate        v4fmulsi  (a)      (b imm)            a.x *= b, a.y *= b, a.z *= b, a.w *= b
            v4dmul          ,    // multiply 4D vector (f64) by another                 v4dmul    (a)      (b)                a.x *= b.x, a.y *= b.y, a.z *= b.z, a.w *= b.w
            v4dmuls         ,    // multiply 4D vector (f64) by scalar                  v4dmuls   (a)      (b)                a.x *= b, a.y *= b, a.z *= b, a.w *= b
            v4dmulsi        ,    // multiply 4D vector (f64) by scalar immediate        v4dmulsi  (a)      (b imm)            a.x *= b, a.y *= b, a.z *= b, a.w *= b
            v4fdiv          ,    // divide 4D vector (f32) by another                   v4fdiv    (a)      (b)                a.x /= b.x, a.y /= b.y, a.z /= b.z, a.w /= b.w
            v4fdivs         ,    // divide 4D vector (f32) by scalar                    v4fdivs   (a)      (b)                a.x /= b, a.y /= b, a.z /= b, a.w /= b
            v4fdivsi        ,    // divide 4D vector (f32) by scalar immediate          v4fdivsi  (a)      (b imm)            a.x /= b, a.y /= b, a.z /= b, a.w /= b
            v4ddiv          ,    // divide 4D vector (f64) by another                   v4ddiv    (a)      (b)                a.x /= b.x, a.y /= b.y, a.z /= b.z, a.w /= b.w
            v4ddivs         ,    // divide 4D vector (f64) by scalar                    v4ddivs   (a)      (b)                a.x /= b, a.y /= b, a.z /= b, a.w /= b
            v4ddivsi        ,    // divide 4D vector (f64) by scalar immediate          v4ddivsi  (a)      (b imm)            a.x /= b, a.y /= b, a.z /= b, a.w /= b
            v4fmod          ,    // modulo 4D vector (f32) by another                   v4fmod    (a)      (b)                a.x %= b.x, a.y %= b.y, a.z %= b.z, a.w %= b.w
            v4fmods         ,    // modulo 4D vector (f32) by scalar                    v4fmods   (a)      (b)                a.x %= b, a.y %= b, a.z %= b, a.w %= b
            v4fmodsi        ,    // modulo 4D vector (f32) by scalar immediate          v4fmodsi  (a)      (b imm)            a.x %= b, a.y %= b, a.z %= b, a.w %= b
            v4dmod          ,    // modulo 4D vector (f64) by another                   v4dmod    (a)      (b)                a.x %= b.x, a.y %= b.y, a.z %= b.z, a.w %= b.w
            v4dmods         ,    // modulo 4D vector (f64) by scalar                    v4dmods   (a)      (b)                a.x %= b, a.y %= b, a.z %= b, a.w %= b
            v4dmodsi        ,    // modulo 4D vector (f64) by scalar immediate          v4dmodsi  (a)      (b imm)            a.x %= b, a.y %= b, a.z %= b, a.w %= b
            v4fneg          ,    // negate 4D vector (f32)                              v4fneg    (a)                         a.x = -a.x, a.y = -a.y, a.z = -a.z, a.w = -a.w
            v4dneg          ,    // negate 4D vector (f64)                              v4dneg    (a)                         a.x = -a.x, a.y = -a.y, a.z = -a.z, a.w = -a.w
            v4fdot          ,    // get dot product of two 4D vectors (f32)             v4fdot    (a)      (b)        (c)     a = dot(b, c)
            v4ddot          ,    // get dot product of two 4D vectors (f64)             v4ddot    (a)      (b)        (c)     a = dot(b, c)
            v4fmag          ,    // get length of 4D vector (f32)                       v4fmag    (a)      (b)                a = length(b)
            v4dmag          ,    // get length of 4D vector (f64)                       v4dmag    (a)      (b)                a = length(b)
            v4fmagsq        ,    // get squared length of 4D vector (f32)               v4fmagsq  (a)      (b)                a = lengthSq(b)
            v4dmagsq        ,    // get squared length of 4D vector (f64)               v4dmagsq  (a)      (b)                a = lengthSq(b)
            v4fnorm         ,    // normalize 4D vector (f32)                           v4fnorm   (a)                         a /= length(a)
            v4dnorm         ,    // normalize 4D vector (f64)                           v4dnorm   (a)                         a /= length(a)
            v4fcross        ,    // get cross product of two 4D vectors (f32)           v4fcross  (a)      (b)        (c)     a.xyz = cross(b.xyz, c.xyz)
            v4dcross        ,    // get cross product of two 4D vectors (f64)           v4dcross  (a)      (b)        (c)     a.xyz = cross(b.xyz, c.xyz)

            // integer arithmetic
            add             ,    // add two registers                                   add       (dest)   (a)         (b)    dest = a + b
            addi            ,    // add register and immediate value                    addi      (dest)   (a)         1.0    dest = a + 1
            sub             ,    // subtract register from another                      sub       (dest)   (a)         (b)    dest = a - b
            subi            ,    // subtract immediate value from register              subi      (dest)   (a)         1.0    dest = a - 1
            subir           ,    // subtract register from immediate value              subir     (dest)   (a)         1.0    dest = 1 - a
            mul             ,    // multiply two registers                              mul       (dest)   (a)         (b)    dest = a * b
            muli            ,    // multiply register and immediate value               muli      (dest)   (a)         1.0    dest = a * 1
            div             ,    // divide register by another                          div       (dest)   (a)         (b)    dest = a / b
            divi            ,    // divide register by immediate value                  divi      (dest)   (a)         1.0    dest = a / 1
            divir           ,    // divide immediate value by register                  divir     (dest)   (a)         1.0    dest = 1 / a
            neg             ,    // negate register and store in another                neg       (dest)   (a)                dest = -a

            // unsigned integer arithmetic
            addu            ,    // add two registers                                   addu      (dest)   (a)         (b)    dest = a + b
            addui           ,    // add register and immediate value                    addui     (dest)   (a)         1.0    dest = a + 1
            subu            ,    // subtract register from another                      subu      (dest)   (a)         (b)    dest = a - b
            subui           ,    // subtract immediate value from register              subui     (dest)   (a)         1.0    dest = a - 1
            subuir          ,    // subtract register from immediate value              subuir    (dest)   (a)         1.0    dest = 1 - a
            mulu            ,    // multiply two registers                              mulu      (dest)   (a)         (b)    dest = a * b
            mului           ,    // multiply register and immediate value               mului     (dest)   (a)         1.0    dest = a * 1
            divu            ,    // divide register by another                          divu      (dest)   (a)         (b)    dest = a / b
            divui           ,    // divide register by immediate value                  divui     (dest)   (a)         1.0    dest = a / 1
            divuir          ,    // divide immediate value by register                  divuir    (dest)   (a)         1.0    dest = 1 / a

            // integer / floating point conversion
            cvt_if          ,    // convert int to f32                                  cvt_if    (a)                         a = f32(a)
            cvt_id          ,    // convert int to f64                                  cvt_id    (a)                         a = f64(a)
            cvt_iu          ,    // convert int to uint                                 cvt_iu    (a)                         a = uint_tp(a)
            cvt_uf          ,    // convert uint to f32                                 cvt_if    (a)                         a = f32(a)
            cvt_ud          ,    // convert uint to f64                                 cvt_id    (a)                         a = f64(a)
            cvt_ui          ,    // convert uint to int                                 cvt_ui    (a)                         a = int_tp(a)
            cvt_fi          ,    // convert f32 to int                                  cvt_fi    (a)                         a = int_tp(a)
            cvt_fu          ,    // convert f32 to uint                                 cvt_fi    (a)                         a = uint_tp(a)
            cvt_fd          ,    // convert f32 to f64                                  cvt_fd    (a)                         a = f64(a)
            cvt_di          ,    // convert f64 to int                                  cvt_di    (a)                         a = int_tp(a)
            cvt_du          ,    // convert f64 to uint                                 cvt_di    (a)                         a = uint_tp(a)
            cvt_df          ,    // convert f64 to f32                                  cvt_df    (a)                         a = f32(a)

            // floating point arithmetic (only accepts $fxx registers and floating point immediates)
            // f32
            fadd            ,    // add two registers                                   fadd      (dest)   (a)         (b)    dest = a + b
            faddi           ,    // add register and immediate value                    faddi     (dest)   (a)         1.0    dest = a + 1.0
            fsub            ,    // subtract register from another                      fsub      (dest)   (a)         (b)    dest = a - b
            fsubi           ,    // subtract immediate value from register              fsubi     (dest)   (a)         1.0    dest = a - 1.0
            fsubir          ,    // subtract register from immediate value              fsubir    (dest)   (a)         1.0    dest = 1.0 - a
            fmul            ,    // multiply two registers                              fmul      (dest)   (a)         (b)    dest = a * b
            fmuli           ,    // multiply register and immediate value               fmuli     (dest)   (a)         1.0    dest = a * 1.0
            fdiv            ,    // divide register by another                          fdiv      (dest)   (a)         (b)    dest = a / b
            fdivi           ,    // divide register by immediate value                  fdivi     (dest)   (a)         1.0    dest = a / 1.0
            fdivir          ,    // divide immediate value by register                  fdivir    (dest)   (a)         1.0    dest = 1.0 / a
            negf            ,    // negate register and store in another                negf      (dest)   (a)                dest = -a
            // f64
            dadd            ,    // add two registers                                   fadd      (dest)   (a)         (b)    dest = a + b
            daddi           ,    // add register and immediate value                    faddi     (dest)   (a)         1.0    dest = a + 1.0
            dsub            ,    // subtract register from another                      fsub      (dest)   (a)         (b)    dest = a - b
            dsubi           ,    // subtract immediate value from register              fsubi     (dest)   (a)         1.0    dest = a - 1.0
            dsubir          ,    // subtract register from immediate value              fsubir    (dest)   (a)         1.0    dest = 1.0 - a
            dmul            ,    // multiply two registers                              fmul      (dest)   (a)         (b)    dest = a * b
            dmuli           ,    // multiply register and immediate value               fmuli     (dest)   (a)         1.0    dest = a * 1.0
            ddiv            ,    // divide register by another                          fdiv      (dest)   (a)         (b)    dest = a / b
            ddivi           ,    // divide register by immediate value                  fdivi     (dest)   (a)         1.0    dest = a / 1.0
            ddivir          ,    // divide immediate value by register                  fdivir    (dest)   (a)         1.0    dest = 1.0 / a
            negd            ,    // negate register and store in another                negd      (dest)   (a)                dest = -a

            // comparison (need unsigned counterparts still)
            lt              ,    // check if register less than register                lt        (dest)   (a)         (b)    dest = a < b
            lti             ,    // check if register less than immediate               lti       (dest)   (a)         1      dest = a < 1
            lte             ,    // check if register less than or equal register       lte       (dest)   (a)         (b)    dest = a <= b
            ltei            ,    // check if register less than or equal immediate      ltei      (dest)   (a)         1      dest = a <= 1
            gt              ,    // check if register greater than register             gt        (dest)   (a)         (b)    dest = a > b
            gti             ,    // check if register greater than immediate            gti       (dest)   (a)         1      dest = a > 1
            gte             ,    // check if register greater than or equal register    gte       (dest)   (a)         (b)    dest = a >= b
            gtei            ,    // check if register greater than or equal imm.        gtei      (dest)   (a)         1      dest = a >= 1
            cmp             ,    // check if register equal register                    cmp       (dest)   (a)         (b)    dest = a == b
            cmpi            ,    // check if register equal immediate                   cmpi      (dest)   (a)         1      dest = a == 1
            ncmp            ,    // check if register not equal register                ncmp      (dest)   (a)         (b)    dest = a != b
            ncmpi           ,    // check if register not equal immediate               ncmpi     (dest)   (a)         1      dest = a != 1

            // floating point comparison
            // f32
            flt             ,    // check if register less than register                flt       (dest)   (a)         (b)    dest = a < b
            flti            ,    // check if register less than immediate               flti      (dest)   (a)         1.0    dest = a < 1.0
            flte            ,    // check if register less than or equal register       flte      (dest)   (a)         (b)    dest = a <= b
            fltei           ,    // check if register less than or equal immediate      fltei     (dest)   (a)         1.0    dest = a <= 1.0
            fgt             ,    // check if register greater than register             fgt       (dest)   (a)         (b)    dest = a > b
            fgti            ,    // check if register greater than immediate            fgti      (dest)   (a)         1.0    dest = a > 1.0
            fgte            ,    // check if register greater than or equal register    fgte      (dest)   (a)         (b)    dest = a >= b
            fgtei           ,    // check if register greater than or equal imm.        fgtei     (dest)   (a)         1.0    dest = a >= 1.0
            fcmp            ,    // check if register equal register                    fcmp      (dest)   (a)         (b)    dest = a == b
            fcmpi           ,    // check if register equal immediate                   fcmpi     (dest)   (a)         1.0    dest = a == 1.0
            fncmp           ,    // check if register not equal register                fncmp     (dest)   (a)         (b)    dest = a != b
            fncmpi          ,    // check if register not equal immediate               fncmpi    (dest)   (a)         1.0    dest = a != 1.0
            // f64
            dlt             ,    // check if register less than register                flt       (dest)   (a)         (b)    dest = a < b
            dlti            ,    // check if register less than immediate               flti      (dest)   (a)         1.0    dest = a < 1.0
            dlte            ,    // check if register less than or equal register       flte      (dest)   (a)         (b)    dest = a <= b
            dltei           ,    // check if register less than or equal immediate      fltei     (dest)   (a)         1.0    dest = a <= 1.0
            dgt             ,    // check if register greater than register             fgt       (dest)   (a)         (b)    dest = a > b
            dgti            ,    // check if register greater than immediate            fgti      (dest)   (a)         1.0    dest = a > 1.0
            dgte            ,    // check if register greater than or equal register    fgte      (dest)   (a)         (b)    dest = a >= b
            dgtei           ,    // check if register greater than or equal imm.        fgtei     (dest)   (a)         1.0    dest = a >= 1.0
            dcmp            ,    // check if register equal register                    fcmp      (dest)   (a)         (b)    dest = a == b
            dcmpi           ,    // check if register equal immediate                   fcmpi     (dest)   (a)         1.0    dest = a == 1.0
            dncmp           ,    // check if register not equal register                fncmp     (dest)   (a)         (b)    dest = a != b
            dncmpi          ,    // check if register not equal immediate               fncmpi    (dest)   (a)         1.0    dest = a != 1.0

            // boolean
            _and            ,    // logical and                                         and       (dest)   (a)         (b)    dest = a && b
            andi            ,    // logical and                                         and       (dest)   (a)         1      dest = a && 1
            _or             ,    // logical or                                          or        (dest)   (a)         (b)    dest = a || b
            ori             ,    // logical or                                          or        (dest)   (a)         1      dest = a || 1

            // bitwise
            band            ,    // bitwise and                                         band      (dest)   (a)         (b)    dest = a & b
            bandi           ,    // bitwise and register and immediate value            bandi     (dest)   (a)         0x0F   dest = a & 0x0F
            bor             ,    // bitwise or                                          bor       (dest)   (a)         (b)    dest = a | b
            bori            ,    // bitwise or register and immediate value             bori      (dest)   (a)         0x0F   dest = a | 0x0F
            _xor            ,    // exclusive or                                        xor       (dest)   (a)         (b)    dest = a ^ b
            xori            ,    // exlusive or register and immediate value            xori      (dest)   (a)         0x0F   dest = a ^ 0x0F
            sl              ,    // shift bits left by amount from register             sl        (dest)   (a)         (b)    dest = a << b
            sli             ,    // shift bits left by immediate value                  sli       (dest)   (a)         4      dest = a << 4
            slir            ,    // shift bits of immediate left by register value      slir      (dest)   (a)         4      dest = 4 << a
            sr              ,    // shift bits right by amount from register            sr        (dest)   (a)         (b)    dest = a >> b
            sri             ,    // shift bits right by immediate value                 sri       (dest)   (a)         4      dest = a >> 4
            srir            ,    // shift bits of immediate right by register value     sri       (dest)   (a)         4      dest = 4 >> a

            // control flow
            beqz            ,    // branch if register equals zero                      beqz      (a)      (fail_addr)        if  a     : goto fail_addr
            bneqz           ,    // branch if register not equals zero                  bneqz     (a)      (fail_addr)        if !a     : goto fail_addr
            bgtz            ,    // branch if register greater than zero                bgtz      (a)      (fail_addr)        if  a <= 0: goto fail_addr
            bgtez           ,    // branch if register greater than or equals zero      bgtez     (a)      (fail_addr)        if  a <  0: goto fail_addr
            bltz            ,    // branch if register less than zero                   bltz      (a)      (fail_addr)        if  a >= 0: goto fail_addr
            bltez           ,    // branch if register less than or equals zero         bltez     (a)      (fail_addr)        if  a >  0: goto fail_addr
            jmp             ,    // jump to address                                     jmp       0x123                       $ip = 0x123
            jmpr            ,    // jump to address in register                         jmp       (a)                         $ip = a
            jal             ,    // jump to address and store $ip in $ra                jal       0x123                       $ra = $ip + 1;$ip = 0x123
            jalr            ,    // jump to address in register and store $ip in $ra    jalr      (a)                         $ra = $ip + 1;$ip = a

            instruction_count    // number of instructions
        };
    };
};