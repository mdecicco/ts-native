#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_vec4.h>
#include <gjs/builtin/script_math.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>

namespace gjs {
    namespace math {
        template <typename T>
        void bind_v4(script_module* mod, const char* name) {
            auto v = mod->bind<vec4<T>>(name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T, T, T>();
            v.constructor<const vec3<T>&>();
            v.constructor<const vec3<T>&, T>();
            v.constructor<T, const vec3<T>&>();
            v.constructor<const vec4<T>&>();
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
            v.method("operator []", &vec4<T>::operator[]);
            v.method("operator -", CONST_METHOD_PTR(vec4<T>, operator -, vec4<T>));
            v.method("dot", &vec4<T>::dot);
            v.method("normalize", &vec4<T>::normalize);
            v.prop("normalized", &vec4<T>::normalized);
            v.prop("length", &vec4<T>::mag);
            v.prop("lengthSq", &vec4<T>::magSq);
            v.prop("xx", &vec4<T>::xx);
            v.prop("xy", &vec4<T>::xy);
            v.prop("xz", &vec4<T>::xz);
            v.prop("xw", &vec4<T>::xw);
            v.prop("yx", &vec4<T>::yx);
            v.prop("yy", &vec4<T>::yy);
            v.prop("yz", &vec4<T>::yz);
            v.prop("yw", &vec4<T>::yw);
            v.prop("zx", &vec4<T>::zx);
            v.prop("zy", &vec4<T>::zy);
            v.prop("zz", &vec4<T>::zz);
            v.prop("zw", &vec4<T>::zw);
            v.prop("wx", &vec4<T>::wx);
            v.prop("wy", &vec4<T>::wy);
            v.prop("wz", &vec4<T>::wz);
            v.prop("ww", &vec4<T>::ww);
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
        void bind_quat(script_module* mod, const char* name) {
            auto v = mod->bind<quat<T>>(name);
            v.constructor<const vec3<T>&, T>();
            v.constructor<const vec3<T>&>();
            v.method("operator *", CONST_METHOD_PTR(quat<T>, operator *, quat<T>, const quat<T>&));
            v.method("operator *=", &quat<T>::operator *=);
            v.method("operator *", CONST_METHOD_PTR(quat<T>, operator *, vec3<T>, const vec3<T>&));
            v.prop("conjugate", &quat<T>::conjugate);
            v.prop("inverse", &quat<T>::inverse);
            v.prop("length", &quat<T>::mag);
            v.prop("lengthSq", &quat<T>::magSq);
            v.prop("x", &quat<T>::x);
            v.prop("y", &quat<T>::y);
            v.prop("z", &quat<T>::z);
            v.prop("w", &quat<T>::w);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;
        }

        template <typename T>
        void bind_m4(script_module* mod, const char* name) {
            auto v = mod->bind<mat4<T>>(name);
            v.constructor();
            v.constructor<T>();
            v.constructor<T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T>();
            v.constructor<const mat3<T>&>();
            v.constructor<const vec4<T>&, const vec4<T>&, const vec4<T>&, const vec4<T>&>();
            v.method("operator +", CONST_METHOD_PTR(mat4<T>, operator +, mat4<T>, const mat4<T>&));
            v.method("operator +=", METHOD_PTR(mat4<T>, operator +=, mat4<T>, const mat4<T>&));
            v.method("operator -", CONST_METHOD_PTR(mat4<T>, operator -, mat4<T>, const mat4<T>&));
            v.method("operator -=", METHOD_PTR(mat4<T>, operator -=, mat4<T>, const mat4<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat4<T>, operator *, mat4<T>, const mat4<T>&));
            v.method("operator *=", METHOD_PTR(mat4<T>, operator *=, mat4<T>, const mat4<T>&));
            v.method("operator /", CONST_METHOD_PTR(mat4<T>, operator /, mat4<T>, T));
            v.method("operator /=", METHOD_PTR(mat4<T>, operator /=, mat4<T>, T));
            v.method("operator *", CONST_METHOD_PTR(mat4<T>, operator *, mat4<T>, T));
            v.method("operator *=", METHOD_PTR(mat4<T>, operator *=, mat4<T>, T));
            v.method("operator *", CONST_METHOD_PTR(mat4<T>, operator *, vec2<T>, const vec2<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat4<T>, operator *, vec3<T>, const vec3<T>&));
            v.method("operator *", CONST_METHOD_PTR(mat4<T>, operator *, vec4<T>, const vec4<T>&));
            v.method("operator []", &mat4<T>::operator[]);
            v.method("rotationX", mat4<T>::rotationX);
            v.method("rotationY", mat4<T>::rotationY);
            v.method("rotationZ", mat4<T>::rotationZ);
            v.method("rotationQuat", mat4<T>::rotationQuat);
            v.method("rotation", mat4<T>::rotation);
            v.method("rotationEuler", mat4<T>::rotationEuler);
            v.method("scale", FUNC_PTR(mat4<T>::scale, mat4<T>, T));
            v.method("scale", FUNC_PTR(mat4<T>::scale, mat4<T>, T, T, T));
            v.method("scale", FUNC_PTR(mat4<T>::scale, mat4<T>, const vec3<T>&));
            v.method("translate3D", mat4<T>::translate3D);
            v.method("compose3D", FUNC_PTR(mat4<T>::compose3D, mat4<T>, const vec3<T>&, const vec3<T>&, const vec3<T>&));
            v.method("compose3D", FUNC_PTR(mat4<T>::compose3D, mat4<T>, const vec3<T>&, T, const vec3<T>&, const vec3<T>&));
            v.method("compose3D", FUNC_PTR(mat4<T>::compose3D, mat4<T>, const quat<T>&, const vec3<T>&, const vec3<T>&));
            v.prop("x", &mat4<T>::x);
            v.prop("y", &mat4<T>::y);
            v.prop("z", &mat4<T>::z);
            v.prop("w", &mat4<T>::w);
            script_type* tp = v.finalize(mod);
            tp->owner = mod;
            tp->is_builtin = true;
        }

        void bind_vec4(script_module* m) {
            bind_v4<f64>(m, "vec4d");
            bind_v4<f32>(m, "vec4f");
            bind_v4<u64>(m, "vec4u64");
            bind_v4<i64>(m, "vec4i64");
            bind_v4<u32>(m, "vec4u32");
            bind_v4<i32>(m, "vec4i32");
            bind_v4<u16>(m, "vec4u16");
            bind_v4<i16>(m, "vec4i16");
            bind_v4<u8>(m, "vec4u8");
            bind_v4<i8>(m, "vec4i8");

            bind_quat<f64>(m, "quatd");
            bind_quat<f32>(m, "quatf");
            bind_quat<u64>(m, "quatu64");
            bind_quat<i64>(m, "quati64");
            bind_quat<u32>(m, "quatu32");
            bind_quat<i32>(m, "quati32");
            bind_quat<u16>(m, "quatu16");
            bind_quat<i16>(m, "quati16");
            bind_quat<u8>(m, "quatu8");
            bind_quat<i8>(m, "quati8");

            bind_m4<f64>(m, "mat4d");
            bind_m4<f32>(m, "mat4f");
            bind_m4<u64>(m, "mat4u64");
            bind_m4<i64>(m, "mat4i64");
            bind_m4<u32>(m, "mat4u32");
            bind_m4<i32>(m, "mat4i32");
            bind_m4<u16>(m, "mat4u16");
            bind_m4<i16>(m, "mat4i16");
            bind_m4<u8>(m, "mat4u8");
            bind_m4<i8>(m, "mat4i8");
        }
    };
};