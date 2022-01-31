#include <gjs/builtin/script_math.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_vec4.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_module.h>
#include <gjs/common/script_object.h>
#include <gjs/bind/bind.h>

#include <gjs/gjs.hpp>

#include <cmath>

float _cos(float x) {
    return cos(x);
}

double _cos(double x) {
    double r = cos(x);
    return r;
}

namespace gjs {
    namespace math {
        template <typename T>
        script_type* tp(script_context* ctx) { return ctx->global()->types()->get<T>(); }

        void bind_math(script_context* ctx) {
            script_module* m = ctx->create_module("math", "builtin/math");

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
                bind<f64, f64>(m, abs, "abs");
                bind<f32, f32>(m, abs, "abs");
                bind<f64, f64>(m, acos, "acos");
                bind<f32, f32>(m, acos, "acos");
                bind<f64, f64>(m, acosh, "acosh");
                bind<f32, f32>(m, acosh, "acosh");
                bind<f64, f64>(m, asin, "asin");
                bind<f32, f32>(m, asin, "asin");
                bind<f64, f64>(m, asinh, "asinh");
                bind<f32, f32>(m, asinh, "asinh");
                bind<f64, f64>(m, atan, "atan");
                bind<f32, f32>(m, atan, "atan");
                bind<f64, f64>(m, atanh, "atanh");
                bind<f32, f32>(m, atanh, "atanh");
                bind<f64, f64, f64>(m, atan2, "atan2");
                bind<f32, f32, f32>(m, atan2f, "atan2");
                bind<f64, f64>(m, cbrt, "cbrt");
                bind<f32, f32>(m, cbrt, "cbrt");
                bind<f64, f64>(m, ceil, "ceil");
                bind<f32, f32>(m, ceil, "ceil");
                //bind<f64, f64>(m, cos, "cos");
                bind<f32, f32>(m, cos, "cos");
                bind<f64, f64>(m, cosh, "cosh");
                bind<f32, f32>(m, cosh, "cosh");
                bind<f64, f64>(m, exp, "exp");
                bind<f32, f32>(m, exp, "exp");
                bind<f64, f64>(m, floor, "floor");
                bind<f32, f32>(m, floor, "floor");
                bind<f64, f64>(m, round, "round");
                bind<f32, f32>(m, round, "round");
                bind<f64, f64>(m, log, "log");
                bind<f32, f32>(m, log, "log");
                bind<f64, f64>(m, log1p, "log1p");
                bind<f32, f32>(m, log1p, "log1p");
                bind<f64, f64>(m, log10, "log10");
                bind<f32, f32>(m, log10, "log10");
                bind<f64, f64>(m, log2, "log2");
                bind<f32, f32>(m, log2, "log2");
                bind<f64, f64, f64>(m, min, "min");
                bind<f32, f32, f32>(m, min, "min");
                bind<i64, i64, i64>(m, min, "min");
                bind<u64, u64, u64>(m, min, "min");
                bind<i32, i32, i32>(m, min, "min");
                bind<u32, u32, u32>(m, min, "min");
                bind<i16, i16, i16>(m, min, "min");
                bind<u16, u16, u16>(m, min, "min");
                bind<i8 , i8 , i8 >(m, min, "min");
                bind<u8 , u8 , u8 >(m, min, "min");
                bind<f64, f64, f64>(m, max, "max");
                bind<f32, f32, f32>(m, max, "max");
                bind<i64, i64, i64>(m, max, "max");
                bind<u64, u64, u64>(m, max, "max");
                bind<i32, i32, i32>(m, max, "max");
                bind<u32, u32, u32>(m, max, "max");
                bind<i16, i16, i16>(m, max, "max");
                bind<u16, u16, u16>(m, max, "max");
                bind<i8 , i8 , i8 >(m, max, "max");
                bind<u8 , u8 , u8 >(m, max, "max");
                bind<f64, f64, f64>(m, pow, "pow");
                bind<f32, f32, f32>(m, powf, "pow");
                bind<f64, f64, f64>(m, random, "random");
                bind<f32, f32, f32>(m, random, "random");
                bind<i64, i64, i64>(m, random, "random");
                bind<u64, u64, u64>(m, random, "random");
                bind<i32, i32, i32>(m, random, "random");
                bind<u32, u32, u32>(m, random, "random");
                bind<i16, i16, i16>(m, random, "random");
                bind<u16, u16, u16>(m, random, "random");
                bind<i8 , i8 , i8 >(m, random, "random");
                bind<u8 , u8 , u8 >(m, random, "random");
                bind<f64, f64>(m, sign, "sign");
                bind<f32, f32>(m, sign, "sign");
                bind<i64, i64>(m, sign, "sign");
                bind<i32, i32>(m, sign, "sign");
                bind<i16, i16>(m, sign, "sign");
                bind<i8 , i8 >(m, sign, "sign");
                //bind<f64, f64>(m, sin, "sin");
                bind<f32, f32>(m, sin, "sin");
                bind<f64, f64>(m, sinh, "sinh");
                bind<f32, f32>(m, sinh, "sinh");
                bind<f64, f64>(m, sqrt, "sqrt");
                bind<f32, f32>(m, sqrt, "sqrt");
                bind<f64, f64>(m, tan, "tan");
                bind<f32, f32>(m, tan, "tan");
                bind<f64, f64>(m, tanh, "tanh");
                bind<f32, f32>(m, tanh, "tanh");
                bind<f64, f64>(m, trunc, "trunc");
                bind<f32, f32>(m, trunc, "trunc");
                bind<f64, f64>(m, r2d, "degrees");
                bind<f32, f32>(m, r2d, "degrees");
                bind<f64, f64>(m, d2r, "radians");
                bind<f32, f32>(m, d2r, "radians");
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
