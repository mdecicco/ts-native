#include <gjs/builtin/script_math.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_vec4.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>

#include <cmath>

namespace gjs {
    namespace math {
        template <typename T>
        script_type* tp(script_context* ctx) { return ctx->global()->types()->get<T>(); }

        void bind_math(script_context* ctx) {
            script_module* m = ctx->create_module("math");

            // constants
            script_type* _f64 = tp<f64>(ctx);
            m->define_local("E", _f64) = 2.71828182845904523536;
            m->define_local("PI", _f64) = 3.14159265358979323846;
            m->define_local("LN10", _f64) = 2.302585092994046;
            m->define_local("LOG2E", _f64) = 1.4426950408889634;
            m->define_local("LOG10E", _f64) = 0.4342944819032518;
            m->define_local("SQRT1_2", _f64) = 0.4342944819032518;
            m->define_local("SQRT2", _f64) = 1.4142135623730951;

            // functions
            {
                m->bind<f64, f64>(abs, "abs");
                m->bind<f32, f32>(abs, "abs");
                m->bind<f64, f64>(acos, "acos");
                m->bind<f32, f32>(acos, "acos");
                m->bind<f64, f64>(acosh, "acosh");
                m->bind<f32, f32>(acosh, "acosh");
                m->bind<f64, f64>(asin, "asin");
                m->bind<f32, f32>(asin, "asin");
                m->bind<f64, f64>(asinh, "asinh");
                m->bind<f32, f32>(asinh, "asinh");
                m->bind<f64, f64>(atan, "atan");
                m->bind<f32, f32>(atan, "atan");
                m->bind<f64, f64>(atanh, "atanh");
                m->bind<f32, f32>(atanh, "atanh");
                m->bind<f64, f64, f64>(atan2, "atan2");
                m->bind<f32, f32, f32>(atan2f, "atan2");
                m->bind<f64, f64>(cbrt, "cbrt");
                m->bind<f32, f32>(cbrt, "cbrt");
                m->bind<f64, f64>(ceil, "ceil");
                m->bind<f32, f32>(ceil, "ceil");
                m->bind<f64, f64>(cos, "cos");
                m->bind<f32, f32>(cos, "cos");
                m->bind<f64, f64>(cosh, "cosh");
                m->bind<f32, f32>(cosh, "cosh");
                m->bind<f64, f64>(exp, "exp");
                m->bind<f32, f32>(exp, "exp");
                m->bind<f64, f64>(floor, "floor");
                m->bind<f32, f32>(floor, "floor");
                m->bind<f64, f64>(round, "round");
                m->bind<f32, f32>(round, "round");
                m->bind<f64, f64>(log, "log");
                m->bind<f32, f32>(log, "log");
                m->bind<f64, f64>(log1p, "log1p");
                m->bind<f32, f32>(log1p, "log1p");
                m->bind<f64, f64>(log10, "log10");
                m->bind<f32, f32>(log10, "log10");
                m->bind<f64, f64>(log2, "log2");
                m->bind<f32, f32>(log2, "log2");
                m->bind<f64, f64, f64>(min, "min");
                m->bind<f32, f32, f32>(min, "min");
                m->bind<i64, i64, i64>(min, "min");
                m->bind<u64, u64, u64>(min, "min");
                m->bind<i32, i32, i32>(min, "min");
                m->bind<u32, u32, u32>(min, "min");
                m->bind<i16, i16, i16>(min, "min");
                m->bind<u16, u16, u16>(min, "min");
                m->bind<i8 , i8 , i8 >(min, "min");
                m->bind<u8 , u8 , u8 >(min, "min");
                m->bind<f64, f64, f64>(max, "max");
                m->bind<f32, f32, f32>(max, "max");
                m->bind<i64, i64, i64>(max, "max");
                m->bind<u64, u64, u64>(max, "max");
                m->bind<i32, i32, i32>(max, "max");
                m->bind<u32, u32, u32>(max, "max");
                m->bind<i16, i16, i16>(max, "max");
                m->bind<u16, u16, u16>(max, "max");
                m->bind<i8 , i8 , i8 >(max, "max");
                m->bind<u8 , u8 , u8 >(max, "max");
                m->bind<f64, f64, f64>(pow, "pow");
                m->bind<f32, f32, f32>(powf, "pow");
                m->bind<f64, f64, f64>(random, "random");
                m->bind<f32, f32, f32>(random, "random");
                m->bind<i64, i64, i64>(random, "random");
                m->bind<u64, u64, u64>(random, "random");
                m->bind<i32, i32, i32>(random, "random");
                m->bind<u32, u32, u32>(random, "random");
                m->bind<i16, i16, i16>(random, "random");
                m->bind<u16, u16, u16>(random, "random");
                m->bind<i8 , i8 , i8 >(random, "random");
                m->bind<u8 , u8 , u8 >(random, "random");
                m->bind<f64, f64>(sign, "sign");
                m->bind<f32, f32>(sign, "sign");
                m->bind<i64, i64>(sign, "sign");
                m->bind<i32, i32>(sign, "sign");
                m->bind<i16, i16>(sign, "sign");
                m->bind<i8 , i8 >(sign, "sign");
                m->bind<f64, f64>(sin, "sin");
                m->bind<f32, f32>(sin, "sin");
                m->bind<f64, f64>(sinh, "sinh");
                m->bind<f32, f32>(sinh, "sinh");
                m->bind<f64, f64>(sqrt, "sqrt");
                m->bind<f32, f32>(sqrt, "sqrt");
                m->bind<f64, f64>(tan, "tan");
                m->bind<f32, f32>(tan, "tan");
                m->bind<f64, f64>(tanh, "tanh");
                m->bind<f32, f32>(tanh, "tanh");
                m->bind<f64, f64>(trunc, "trunc");
                m->bind<f32, f32>(trunc, "trunc");
                m->bind<f64, f64>(r2d, "degrees");
                m->bind<f32, f32>(r2d, "degrees");
                m->bind<f64, f64>(d2r, "radians");
                m->bind<f32, f32>(d2r, "radians");
            }

            // classes
            {
                bind_vec2(m);
                bind_vec3(m);
                bind_vec4(m);
            }
        }
    };
};