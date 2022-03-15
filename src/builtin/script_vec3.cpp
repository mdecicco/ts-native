#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_math.h>

#include <gjs/gjs.hpp>

namespace gjs {
    namespace math {
        template <typename T>
        void bind_v3(script_module* mod, const char* name) {
            auto v = bind<vec3<T>>(mod, name);
            v.constructor();
            v.template constructor<T>();
            v.template constructor<T, T, T>();
            v.template constructor<const vec2<T>&>();
            v.template constructor<const vec2<T>&, T>();
            v.template constructor<T, const vec2<T>&>();
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
            v.method("operator []", &vec3<T>::operator[]);
            v.method("operator -", CONST_METHOD_PTR(vec3<T>, operator -, vec3<T>));
            v.method("operator =", &vec3<T>::operator=);
            v.method("distance", &vec3<T>::distance);
            v.method("distanceSq", &vec3<T>::distanceSq);
            v.method("dot", &vec3<T>::dot);
            v.method("cross", &vec3<T>::cross);
            v.method("normalize", &vec3<T>::normalize);
            v.prop("x", &vec3<T>::x);
            v.prop("y", &vec3<T>::y);
            v.prop("z", &vec3<T>::z);
            v.prop("length", &vec3<T>::mag);
            v.prop("lengthSq", &vec3<T>::magSq);
            v.prop("normalized", &vec3<T>::normalized);
            v.prop("xx", &vec3<T>::xx);
            v.prop("xy", &vec3<T>::xy);
            v.prop("xz", &vec3<T>::xz);
            v.prop("yx", &vec3<T>::yx);
            v.prop("yy", &vec3<T>::yy);
            v.prop("yz", &vec3<T>::yz);
            v.prop("zx", &vec3<T>::zx);
            v.prop("zy", &vec3<T>::zy);
            v.prop("zz", &vec3<T>::zz);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;
            tp->is_trivially_copyable = true;

            bind<vec3<T>, const vec3<T>&, const vec3<T>&>(mod, min, "min");
            bind<vec3<T>, const vec3<T>&, const vec3<T>&>(mod, max, "max");
            bind<vec3<T>, const vec3<T>&, const vec3<T>&>(mod, random, "random");
        }

        template <typename T>
        void bind_m3(script_module* mod, const char* name) {
            auto v = bind<mat3<T>>(mod, name);
            v.constructor();
            v.template constructor<T>();
            v.template constructor<T, T, T, T, T, T, T, T, T>();
            v.template constructor<const vec3<T>&, const vec3<T>&, const vec3<T>&>();
            v.template constructor<const mat3<T>&>();
            v.method("operator +", CONST_METHOD_PTR(mat3<T>, operator +, mat3<T>, const mat3<T>&));
            v.method("operator +=", METHOD_PTR(mat3<T>, operator +=, mat3<T>, const mat3<T>&));
            v.method("operator -", CONST_METHOD_PTR(mat3<T>, operator -, mat3<T>, const mat3<T>&));
            v.method("operator -=", METHOD_PTR(mat3<T>, operator -=, mat3<T>, const mat3<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat3<T>, operator *, mat3<T>, const mat3<T>&));
            v.method("operator *=", METHOD_PTR(mat3<T>, operator *=, mat3<T>, const mat3<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat3<T>, operator *, mat3<T>, T));
            v.method("operator *=", METHOD_PTR(mat3<T>, operator *=, mat3<T>, T));
            v.method("operator /", CONST_METHOD_PTR(mat3<T>, operator /, mat3<T>, T));
            v.method("operator /=", METHOD_PTR(mat3<T>, operator /=, mat3<T>, T));
            v.method("operator *", CONST_METHOD_PTR(mat3<T>, operator *, vec2<T>, const vec2<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat3<T>, operator *, vec3<T>, const vec3<T>&));
            v.method("operator []", &mat3<T>::operator[]);
            v.method("col", &mat3<T>::col);
            v.method("rotationX", mat3<T>::rotationX);
            v.method("rotationY", mat3<T>::rotationY);
            v.method("rotationZ", mat3<T>::rotationZ);
            v.method("rotationEuler", mat3<T>::rotationEuler);
            v.method("scale", FUNC_PTR(mat3<T>::scale, mat3<T>, T));
            v.method("scale", FUNC_PTR(mat3<T>::scale, mat3<T>, T, T, T));
            v.method("scale", FUNC_PTR(mat3<T>::scale, mat3<T>, const vec3<T>&));
            v.method("translate2D", mat3<T>::translate2D);
            v.method("compose2D", mat3<T>::compose2D);
            v.prop("x", &mat3<T>::x);
            v.prop("y", &mat3<T>::y);
            v.prop("z", &mat3<T>::z);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;
        }

        void bind_vec3(script_module* m) {
            bind_v3<f64>(m, "vec3d");
            bind_v3<f32>(m, "vec3f");
            //bind_v3<u64>(m, "vec3u64");
            bind_v3<i64>(m, "vec3i64");
            //bind_v3<u32>(m, "vec3u32");
            bind_v3<i32>(m, "vec3i32");
            //bind_v3<u16>(m, "vec3u16");
            bind_v3<i16>(m, "vec3i16");
            //bind_v3<u8>(m, "vec3u8");
            bind_v3<i8>(m, "vec3i8");

            bind_m3<f64>(m, "mat3d");
            bind_m3<f32>(m, "mat3f");
            //bind_m3<u64>(m, "mat3u64");
            bind_m3<i64>(m, "mat3i64");
            //bind_m3<u32>(m, "mat3u32");
            bind_m3<i32>(m, "mat3i32");
            //bind_m3<u16>(m, "mat3u16");
            bind_m3<i16>(m, "mat3i16");
            //bind_m3<u8>(m, "mat3u8");
            bind_m3<i8>(m, "mat3i8");
        }
    };
};
