#include <gjs/builtin/script_math.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <cmath>

namespace gjs {
    namespace math {
        template <typename T>
        void bind_vec2(script_module* mod, const char* name) {
            auto v = mod->bind<vec2<T>>(name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T>();
            v.method("operator +", CONST_METHOD_PTR(vec2<T>, operator +, vec2<T>, const vec2<T>&));
            v.method("operator +", CONST_METHOD_PTR(vec2<T>, operator +, vec2<T>, T));
            v.method("operator +=", METHOD_PTR(vec2<T>, operator +=, vec2<T>, const vec2<T>&));
            v.method("operator +=", METHOD_PTR(vec2<T>, operator +=, vec2<T>, T));
            v.method("operator -", CONST_METHOD_PTR(vec2<T>, operator -, vec2<T>, const vec2<T>&));
            v.method("operator -", CONST_METHOD_PTR(vec2<T>, operator -, vec2<T>, T));
            v.method("operator -=", METHOD_PTR(vec2<T>, operator -=, vec2<T>, const vec2<T>&));
            v.method("operator -=", METHOD_PTR(vec2<T>, operator -=, vec2<T>, T));
            v.method("operator *", CONST_METHOD_PTR(vec2<T>, operator *, vec2<T>, const vec2<T>&));
            v.method("operator *", CONST_METHOD_PTR(vec2<T>, operator *, vec2<T>, T));
            v.method("operator *=", METHOD_PTR(vec2<T>, operator *=, vec2<T>, const vec2<T>&));
            v.method("operator *=", METHOD_PTR(vec2<T>, operator *=, vec2<T>, T));
            v.method("operator /", CONST_METHOD_PTR(vec2<T>, operator /, vec2<T>, const vec2<T>&));
            v.method("operator /", CONST_METHOD_PTR(vec2<T>, operator /, vec2<T>, T));
            v.method("operator /=", METHOD_PTR(vec2<T>, operator /=, vec2<T>, const vec2<T>&));
            v.method("operator /=", METHOD_PTR(vec2<T>, operator /=, vec2<T>, T));
            v.method("distance", &vec2<T>::distance);
            v.method("distanceSq", &vec2<T>::distanceSq);
            v.method("dot", &vec2<T>::dot);
            v.method("normalize", &vec2<T>::normalize);
            v.method("fromAngleLength", &vec2<T>::fromAngleMag);
            v.method("fromAngleLengthr", &vec2<T>::fromAngleMag_r);
            v.prop("normalized", &vec2<T>::normalized);
            v.prop("length", &vec2<T>::mag);
            v.prop("lengthSq", &vec2<T>::magSq);
            v.prop("x", &vec2<T>::x);
            v.prop("y", &vec2<T>::y);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;

            mod->bind<vec2<T>, const vec2<T>&, const vec2<T>&>(min, "min");
            mod->bind<vec2<T>, const vec2<T>&, const vec2<T>&>(max, "max");
            mod->bind<vec2<T>, const vec2<T>&, const vec2<T>&>(random, "random");
        }

        template <typename T>
        void bind_vec3(script_module* mod, const char* name) {
            auto v = mod->bind<vec3<T>>(name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T, T>();
            v.constructor<const vec2<T>&, T>();
            v.constructor<T, const vec2<T>&>();
            v.method("operator +", CONST_METHOD_PTR(vec3<T>, operator +, vec3<T>, const vec3<T>&));
            v.method("operator +", CONST_METHOD_PTR(vec3<T>, operator +, vec3<T>, T));
            v.method("operator +=", METHOD_PTR(vec3<T>, operator +=, vec3<T>, const vec3<T>&));
            v.method("operator +=", METHOD_PTR(vec3<T>, operator +=, vec3<T>, T));
            v.method("operator -", CONST_METHOD_PTR(vec3<T>, operator -, vec3<T>, const vec3<T>&));
            v.method("operator -", CONST_METHOD_PTR(vec3<T>, operator -, vec3<T>, T));
            v.method("operator -=", METHOD_PTR(vec3<T>, operator -=, vec3<T>, const vec3<T>&));
            v.method("operator -=", METHOD_PTR(vec3<T>, operator -=, vec3<T>, T));
            v.method("operator *", CONST_METHOD_PTR(vec3<T>, operator *, vec3<T>, const vec3<T>&));
            v.method("operator *", CONST_METHOD_PTR(vec3<T>, operator *, vec3<T>, T));
            v.method("operator *=", METHOD_PTR(vec3<T>, operator *=, vec3<T>, const vec3<T>&));
            v.method("operator *=", METHOD_PTR(vec3<T>, operator *=, vec3<T>, T));
            v.method("operator /", CONST_METHOD_PTR(vec3<T>, operator /, vec3<T>, const vec3<T>&));
            v.method("operator /", CONST_METHOD_PTR(vec3<T>, operator /, vec3<T>, T));
            v.method("operator /=", METHOD_PTR(vec3<T>, operator /=, vec3<T>, const vec3<T>&));
            v.method("operator /=", METHOD_PTR(vec3<T>, operator /=, vec3<T>, T));
            v.method("distance", &vec3<T>::distance);
            v.method("distanceSq", &vec3<T>::distanceSq);
            v.method("dot", &vec3<T>::dot);
            v.method("cross", &vec3<T>::cross);
            v.method("normalize", &vec3<T>::normalize);
            v.prop("normalized", &vec3<T>::normalized);
            v.prop("length", &vec3<T>::mag);
            v.prop("lengthSq", &vec3<T>::magSq);
            v.prop("xx", &vec3<T>::xx);
            v.prop("xy", &vec3<T>::xy);
            v.prop("xz", &vec3<T>::xz);
            v.prop("yx", &vec3<T>::yx);
            v.prop("yy", &vec3<T>::yy);
            v.prop("yz", &vec3<T>::yz);
            v.prop("zx", &vec3<T>::zx);
            v.prop("zy", &vec3<T>::zy);
            v.prop("zz", &vec3<T>::zz);
            v.prop("x", &vec3<T>::x);
            v.prop("y", &vec3<T>::y);
            v.prop("z", &vec3<T>::z);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;

            mod->bind<vec3<T>, const vec3<T>&, const vec3<T>&>(min, "min");
            mod->bind<vec3<T>, const vec3<T>&, const vec3<T>&>(max, "max");
            mod->bind<vec3<T>, const vec3<T>&, const vec3<T>&>(random, "random");
        }

        template <typename T>
        void bind_vec4(script_module* mod, const char* name) {
            auto v = mod->bind<vec4<T>>(name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T, T, T>();
            v.constructor<const vec3<T>&, T>();
            v.constructor<T, const vec3<T>&>();
            v.method("operator +", CONST_METHOD_PTR(vec4<T>, operator +, vec4<T>, const vec4<T>&));
            v.method("operator +", CONST_METHOD_PTR(vec4<T>, operator +, vec4<T>, T));
            v.method("operator +=", METHOD_PTR(vec4<T>, operator +=, vec4<T>, const vec4<T>&));
            v.method("operator +=", METHOD_PTR(vec4<T>, operator +=, vec4<T>, T));
            v.method("operator -", CONST_METHOD_PTR(vec4<T>, operator -, vec4<T>, const vec4<T>&));
            v.method("operator -", CONST_METHOD_PTR(vec4<T>, operator -, vec4<T>, T));
            v.method("operator -=", METHOD_PTR(vec4<T>, operator -=, vec4<T>, const vec4<T>&));
            v.method("operator -=", METHOD_PTR(vec4<T>, operator -=, vec4<T>, T));
            v.method("operator *", CONST_METHOD_PTR(vec4<T>, operator *, vec4<T>, const vec4<T>&));
            v.method("operator *", CONST_METHOD_PTR(vec4<T>, operator *, vec4<T>, T));
            v.method("operator *=", METHOD_PTR(vec4<T>, operator *=, vec4<T>, const vec4<T>&));
            v.method("operator *=", METHOD_PTR(vec4<T>, operator *=, vec4<T>, T));
            v.method("operator /", CONST_METHOD_PTR(vec4<T>, operator /, vec4<T>, const vec4<T>&));
            v.method("operator /", CONST_METHOD_PTR(vec4<T>, operator /, vec4<T>, T));
            v.method("operator /=", METHOD_PTR(vec4<T>, operator /=, vec4<T>, const vec4<T>&));
            v.method("operator /=", METHOD_PTR(vec4<T>, operator /=, vec4<T>, T));
            v.method("dot", &vec4<T>::dot);
            v.method("normalize", &vec4<T>::normalize);
            v.prop("normalized", &vec4<T>::normalized);
            v.prop("length", &vec4<T>::mag);
            v.prop("lengthSq", &vec4<T>::magSq);
            v.prop("xxx", &vec4<T>::xxx);
            v.prop("xxy", &vec4<T>::xxy);
            v.prop("xxz", &vec4<T>::xxz);
            v.prop("xxw", &vec4<T>::xxw);
            v.prop("xyx", &vec4<T>::xyx);
            v.prop("xyy", &vec4<T>::xyy);
            v.prop("xyz", &vec4<T>::xyz);
            v.prop("xyw", &vec4<T>::xyw);
            v.prop("xzx", &vec4<T>::xzx);
            v.prop("xzy", &vec4<T>::xzy);
            v.prop("xzz", &vec4<T>::xzz);
            v.prop("xzw", &vec4<T>::xzw);
            v.prop("xwx", &vec4<T>::xwx);
            v.prop("xwy", &vec4<T>::xwy);
            v.prop("xwz", &vec4<T>::xwz);
            v.prop("xww", &vec4<T>::xww);
            v.prop("yxx", &vec4<T>::yxx);
            v.prop("yxy", &vec4<T>::yxy);
            v.prop("yxz", &vec4<T>::yxz);
            v.prop("yxw", &vec4<T>::yxw);
            v.prop("yyx", &vec4<T>::yyx);
            v.prop("yyy", &vec4<T>::yyy);
            v.prop("yyz", &vec4<T>::yyz);
            v.prop("yyw", &vec4<T>::yyw);
            v.prop("yzx", &vec4<T>::yzx);
            v.prop("yzy", &vec4<T>::yzy);
            v.prop("yzz", &vec4<T>::yzz);
            v.prop("yzw", &vec4<T>::yzw);
            v.prop("ywx", &vec4<T>::ywx);
            v.prop("ywy", &vec4<T>::ywy);
            v.prop("ywz", &vec4<T>::ywz);
            v.prop("yww", &vec4<T>::yww);
            v.prop("zxx", &vec4<T>::zxx);
            v.prop("zxy", &vec4<T>::zxy);
            v.prop("zxz", &vec4<T>::zxz);
            v.prop("zxw", &vec4<T>::zxw);
            v.prop("zyx", &vec4<T>::zyx);
            v.prop("zyy", &vec4<T>::zyy);
            v.prop("zyz", &vec4<T>::zyz);
            v.prop("zyw", &vec4<T>::zyw);
            v.prop("zzx", &vec4<T>::zzx);
            v.prop("zzy", &vec4<T>::zzy);
            v.prop("zzz", &vec4<T>::zzz);
            v.prop("zzw", &vec4<T>::zzw);
            v.prop("zwx", &vec4<T>::zwx);
            v.prop("zwy", &vec4<T>::zwy);
            v.prop("zwz", &vec4<T>::zwz);
            v.prop("zww", &vec4<T>::zww);
            v.prop("wxx", &vec4<T>::wxx);
            v.prop("wxy", &vec4<T>::wxy);
            v.prop("wxz", &vec4<T>::wxz);
            v.prop("wxw", &vec4<T>::wxw);
            v.prop("wyx", &vec4<T>::wyx);
            v.prop("wyy", &vec4<T>::wyy);
            v.prop("wyz", &vec4<T>::wyz);
            v.prop("wyw", &vec4<T>::wyw);
            v.prop("wzx", &vec4<T>::wzx);
            v.prop("wzy", &vec4<T>::wzy);
            v.prop("wzz", &vec4<T>::wzz);
            v.prop("wzw", &vec4<T>::wzw);
            v.prop("wwx", &vec4<T>::wwx);
            v.prop("wwy", &vec4<T>::wwy);
            v.prop("wwz", &vec4<T>::wwz);
            v.prop("www", &vec4<T>::www);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;

            mod->bind<vec4<T>, const vec4<T>&, const vec4<T>&>(min, "min");
            mod->bind<vec4<T>, const vec4<T>&, const vec4<T>&>(max, "max");
            mod->bind<vec4<T>, const vec4<T>&, const vec4<T>&>(random, "random");
        }

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
                // vec2
                bind_vec2<f64>(m, "vec2d");
                bind_vec2<f32>(m, "vec2f");
                bind_vec2<u64>(m, "vec2u64");
                bind_vec2<i64>(m, "vec2i64");
                bind_vec2<u32>(m, "vec2u32");
                bind_vec2<i32>(m, "vec2i32");
                bind_vec2<u16>(m, "vec2u16");
                bind_vec2<i16>(m, "vec2i16");
                bind_vec2<u8>(m, "vec2u8");
                bind_vec2<i8>(m, "vec2i8");

                // vec3
                bind_vec3<f64>(m, "vec3d");
                bind_vec3<f32>(m, "vec3f");
                bind_vec3<u64>(m, "vec3u64");
                bind_vec3<i64>(m, "vec3i64");
                bind_vec3<u32>(m, "vec3u32");
                bind_vec3<i32>(m, "vec3i32");
                bind_vec3<u16>(m, "vec3u16");
                bind_vec3<i16>(m, "vec3i16");
                bind_vec3<u8>(m, "vec3u8");
                bind_vec3<i8>(m, "vec3i8");

                // vec4
                bind_vec4<f64>(m, "vec4d");
                bind_vec4<f32>(m, "vec4f");
                bind_vec4<u64>(m, "vec4u64");
                bind_vec4<i64>(m, "vec4i64");
                bind_vec4<u32>(m, "vec4u32");
                bind_vec4<i32>(m, "vec4i32");
                bind_vec4<u16>(m, "vec4u16");
                bind_vec4<i16>(m, "vec4i16");
                bind_vec4<u8>(m, "vec4u8");
                bind_vec4<i8>(m, "vec4i8");
            }
        }
    };
};