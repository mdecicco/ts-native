#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_math.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/bind/bind.h>

namespace gjs {
    namespace math {
        template <typename T>
        void bind_v2(script_module* mod, const char* name) {
            auto v = bind<vec2<T>>(mod, name);
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
            v.method("operator []", &vec2<T>::operator[]);
            v.method("operator -", CONST_METHOD_PTR(vec2<T>, operator -, vec2<T>));
            v.method("operator =", &vec2<T>::operator=);
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
            tp->is_trivially_copyable = true;

            bind<vec2<T>, const vec2<T>&, const vec2<T>&>(mod, min, "min");
            bind<vec2<T>, const vec2<T>&, const vec2<T>&>(mod, max, "max");
            bind<vec2<T>, const vec2<T>&, const vec2<T>&>(mod, random, "random");
        }

        template <typename T>
        void bind_m2(script_module* mod, const char* name) {
            auto v = bind<mat2<T>>(mod, name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T, T, T>();
            v.constructor<const vec2<T>&, const vec2<T>&>();
            v.method("operator +", CONST_METHOD_PTR(mat2<T>, operator +, mat2<T>, const mat2<T>&));
            v.method("operator +=", METHOD_PTR(mat2<T>, operator +=, mat2<T>, const mat2<T>&));
            v.method("operator -", CONST_METHOD_PTR(mat2<T>, operator -, mat2<T>, const mat2<T>&));
            v.method("operator -=", METHOD_PTR(mat2<T>, operator -=, mat2<T>, const mat2<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat2<T>, operator *, mat2<T>, const mat2<T>&));
            v.method("operator *=", METHOD_PTR(mat2<T>, operator *=, mat2<T>, const mat2<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat2<T>, operator *, mat2<T>, T));
            v.method("operator *=", METHOD_PTR(mat2<T>, operator *=, mat2<T>, T));
            v.method("operator /", CONST_METHOD_PTR(mat2<T>, operator /, mat2<T>, T));
            v.method("operator /=", METHOD_PTR(mat2<T>, operator /=, mat2<T>, T));
            v.method("operator *", CONST_METHOD_PTR(mat2<T>, operator *, vec2<T>, const vec2<T>&));
            v.method("operator []", &mat2<T>::operator[]);
            v.method("col", &mat2<T>::col);
            v.method("rotation", mat2<T>::rotation);
            v.method("scale", FUNC_PTR(mat2<T>::scale, mat2<T>, T));
            v.method("scale", FUNC_PTR(mat2<T>::scale, mat2<T>, T, T));
            v.method("scale", FUNC_PTR(mat2<T>::scale, mat2<T>, const vec2<T>&));
            v.prop("x", &mat2<T>::x);
            v.prop("y", &mat2<T>::y);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;
        }

        void bind_vec2(script_module* m) {
            bind_v2<f64>(m, "vec2d");
            bind_v2<f32>(m, "vec2f");
            //bind_v2<u64>(m, "vec2u64");
            bind_v2<i64>(m, "vec2i64");
            //bind_v2<u32>(m, "vec2u32");
            bind_v2<i32>(m, "vec2i32");
            //bind_v2<u16>(m, "vec2u16");
            bind_v2<i16>(m, "vec2i16");
            //bind_v2<u8>(m, "vec2u8");
            bind_v2<i8>(m, "vec2i8");

            bind_m2<f64>(m, "mat2d");
            bind_m2<f32>(m, "mat2f");
            //bind_m2<u64>(m, "mat2u64");
            bind_m2<i64>(m, "mat2i64");
            //bind_m2<u32>(m, "mat2u32");
            bind_m2<i32>(m, "mat2i32");
            //bind_m2<u16>(m, "mat2u16");
            bind_m2<i16>(m, "mat2i16");
            //bind_m2<u8>(m, "mat2u8");
            bind_m2<i8>(m, "mat2i8");
        }
    };
};