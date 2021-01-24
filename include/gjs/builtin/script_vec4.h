#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_module;

    namespace math {
        template <typename T>
        class vec4 {
            public:
                vec4() : x(T(0)), y(T(0)), z(T(0)), w(T(0)) {}
                vec4(T v) : x(v), y(v), z(v), w(v) {}
                vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
                vec4(const vec3<T>& xyz) : x(xyz.x), y(xyz.y), z(xyz.z), w(T(0)) {}
                vec4(const vec3<T>& xyz, T _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}
                vec4(T _x, const vec3<T>& yzw) : x(_x), y(yzw.x), z(yzw.y), w(yzw.z) {}

                vec4<T> operator + (const vec4<T>& rhs) const { return { T(x + rhs.x), T(y + rhs.y), T(z + rhs.z), T(w + rhs.w) }; }
                vec4<T> operator += (const vec4<T>& rhs) { return { T(x += rhs.x), T(y += rhs.y), T(z += rhs.z), T(w += rhs.w) }; }
                vec4<T> operator - (const vec4<T>& rhs) const { return { T(x - rhs.x), T(y - rhs.y), T(z - rhs.z), T(z - rhs.z) }; }
                vec4<T> operator -= (const vec4<T>& rhs) { return { T(x -= rhs.x), T(y -= rhs.y), T(z -= rhs.z), T(w -= rhs.w) }; }
                vec4<T> operator * (const vec4<T>& rhs) const { return { T(x * rhs.x), T(y * rhs.y), T(z * rhs.z), T(w * rhs.w) }; }
                vec4<T> operator *= (const vec4<T>& rhs) { return { T(x *= rhs.x), T(y *= rhs.y), T(z *= rhs.z), T(w *= rhs.w) }; }
                vec4<T> operator / (const vec4<T>& rhs) const { return { T(x / rhs.x), T(y / rhs.y), T(y / rhs.z), T(w / rhs.w) }; }
                vec4<T> operator /= (const vec4<T>& rhs) { return { T(x /= rhs.x), T(y /= rhs.y), T(y /= rhs.z), T(w /= rhs.w) }; }
                vec4<T> operator + (T rhs) const { return { T(x + rhs), T(y + rhs), T(z + rhs), T(w + rhs) }; }
                vec4<T> operator += (T rhs) { return { T(x += rhs), T(y += rhs), T(z += rhs), T(w += rhs) }; }
                vec4<T> operator - (T rhs) const { return { T(x - rhs), T(y - rhs), T(z - rhs), T(w - rhs) }; }
                vec4<T> operator -= (T rhs) { return { T(x -= rhs), T(y -= rhs), T(z -= rhs), T(w -= rhs) }; }
                vec4<T> operator * (T rhs) const { return { T(x * rhs), T(y * rhs), T(z * rhs), T(w * rhs) }; }
                vec4<T> operator *= (T rhs) { return { T(x *= rhs), T(y *= rhs), T(z *= rhs), T(w *= rhs) }; }
                vec4<T> operator / (T rhs) const { return { T(x / rhs), T(y / rhs), T(z / rhs), T(w / rhs) }; }
                vec4<T> operator /= (T rhs) { return { T(x /= rhs), T(y /= rhs), T(z /= rhs), T(w /= rhs) }; }
                vec4<T> operator - () const { return { T(-x), T(-y), T(-z), T(-w) }; }
                T& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 3
                    return (&x)[idx];
                }

                T mag() const { return sqrt((x * x) + (y * y) + (z * z) + (w * w)); }
                T magSq() const { return (x * x) + (y * y) + (z * z) + (w * w); }
                T dot(const vec4<T>& v) const { return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w); }
                vec4<T> normalized() const {
                    T invMag = T(1.0) / mag();
                    return { T(x * invMag), T(y * invMag), T(z * invMag), T(w * invMag) };
                }
                void normalize() {
                    T invMag = T(1.0) / mag();
                    x *= invMag; y *= invMag; z *= invMag; w * invMag;
                }

                vec2<T> xx() const { return { x, x }; }
                vec2<T> xy() const { return { x, y }; }
                vec2<T> xz() const { return { x, z }; }
                vec2<T> xw() const { return { x, w }; }
                vec2<T> yx() const { return { y, x }; }
                vec2<T> yy() const { return { y, y }; }
                vec2<T> yz() const { return { y, z }; }
                vec2<T> yw() const { return { y, w }; }
                vec2<T> zx() const { return { z, x }; }
                vec2<T> zy() const { return { z, y }; }
                vec2<T> zz() const { return { z, z }; }
                vec2<T> zw() const { return { z, w }; }
                vec2<T> wx() const { return { w, x }; }
                vec2<T> wy() const { return { w, y }; }
                vec2<T> wz() const { return { w, z }; }
                vec2<T> ww() const { return { w, w }; }
                vec3<T> xxx() const { return { x, x, x }; }
                vec3<T> xxy() const { return { x, x, y }; }
                vec3<T> xxz() const { return { x, x, z }; }
                vec3<T> xxw() const { return { x, x, w }; }
                vec3<T> xyx() const { return { x, y, x }; }
                vec3<T> xyy() const { return { x, y, y }; }
                vec3<T> xyz() const { return { x, y, z }; }
                vec3<T> xyw() const { return { x, y, w }; }
                vec3<T> xzx() const { return { x, z, x }; }
                vec3<T> xzy() const { return { x, z, y }; }
                vec3<T> xzz() const { return { x, z, z }; }
                vec3<T> xzw() const { return { x, z, w }; }
                vec3<T> xwx() const { return { x, w, x }; }
                vec3<T> xwy() const { return { x, w, y }; }
                vec3<T> xwz() const { return { x, w, z }; }
                vec3<T> xww() const { return { x, w, w }; }
                vec3<T> yxx() const { return { y, x, x }; }
                vec3<T> yxy() const { return { y, x, y }; }
                vec3<T> yxz() const { return { y, x, z }; }
                vec3<T> yxw() const { return { y, x, w }; }
                vec3<T> yyx() const { return { y, y, x }; }
                vec3<T> yyy() const { return { y, y, y }; }
                vec3<T> yyz() const { return { y, y, z }; }
                vec3<T> yyw() const { return { y, y, w }; }
                vec3<T> yzx() const { return { y, z, x }; }
                vec3<T> yzy() const { return { y, z, y }; }
                vec3<T> yzz() const { return { y, z, z }; }
                vec3<T> yzw() const { return { y, z, w }; }
                vec3<T> ywx() const { return { y, w, x }; }
                vec3<T> ywy() const { return { y, w, y }; }
                vec3<T> ywz() const { return { y, w, z }; }
                vec3<T> yww() const { return { y, w, w }; }
                vec3<T> zxx() const { return { z, x, x }; }
                vec3<T> zxy() const { return { z, x, y }; }
                vec3<T> zxz() const { return { z, x, z }; }
                vec3<T> zxw() const { return { z, x, w }; }
                vec3<T> zyx() const { return { z, y, x }; }
                vec3<T> zyy() const { return { z, y, y }; }
                vec3<T> zyz() const { return { z, y, z }; }
                vec3<T> zyw() const { return { z, y, w }; }
                vec3<T> zzx() const { return { z, z, x }; }
                vec3<T> zzy() const { return { z, z, y }; }
                vec3<T> zzz() const { return { z, z, z }; }
                vec3<T> zzw() const { return { z, z, w }; }
                vec3<T> zwx() const { return { z, w, x }; }
                vec3<T> zwy() const { return { z, w, y }; }
                vec3<T> zwz() const { return { z, w, z }; }
                vec3<T> zww() const { return { z, w, w }; }
                vec3<T> wxx() const { return { w, x, x }; }
                vec3<T> wxy() const { return { w, x, y }; }
                vec3<T> wxz() const { return { w, x, z }; }
                vec3<T> wxw() const { return { w, x, w }; }
                vec3<T> wyx() const { return { w, y, x }; }
                vec3<T> wyy() const { return { w, y, y }; }
                vec3<T> wyz() const { return { w, y, z }; }
                vec3<T> wyw() const { return { w, y, w }; }
                vec3<T> wzx() const { return { w, z, x }; }
                vec3<T> wzy() const { return { w, z, y }; }
                vec3<T> wzz() const { return { w, z, z }; }
                vec3<T> wzw() const { return { w, z, w }; }
                vec3<T> wwx() const { return { w, w, x }; }
                vec3<T> wwy() const { return { w, w, y }; }
                vec3<T> wwz() const { return { w, w, z }; }
                vec3<T> www() const { return { w, w, w }; }

                T x, y, z, w;
        };

        template <typename T>
        class quat {
            public:
                quat(const vec3<T>& axis, T angleDeg) {
                    T Hrad = d2r(angleDeg * T(0.5));
                    T sHrad = sin(Hrad);
                    x = axis.x * sHrad;
                    y = axis.y * sHrad;
                    z = axis.z * sHrad;
                    w = cos(Hrad);
                }
                quat(const vec3<T>& eulerDeg) {
                    vec3<T> rad(d2r(eulerDeg.x), d2r(eulerDeg.y), d2r(eulerDeg.z));
                    T c1 = cos(rad.z * T(0.5));
                    T c2 = cos(rad.y * T(0.5));
                    T c3 = cos(rad.x * T(0.5));
                    T s1 = sin(rad.z * T(0.5));
                    T s2 = sin(rad.y * T(0.5));
                    T s3 = sin(rad.x * T(0.5));

                    x = c1 * c2 * s3 - s1 * s2 * c3;
                    y = c1 * s2 * c3 + s1 * c2 * s3;
                    z = s1 * c2 * c3 - c1 * s2 * s3;
                    w = c1 * c2 * c3 + s1 * s2 * s3;
                }
                quat(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) { }

                quat<T> operator * (const quat<T>& rhs) const {
                    return {
                        T(y * rhs.z - z * rhs.y + x * rhs.w + w * rhs.x),
                        T(z * rhs.x - x * rhs.z + y * rhs.w + w * rhs.y),
                        T(x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z),
                        T(w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z)
                    };
                }
                quat<T> operator *= (const quat<T>& rhs) {
                    quat<T> r = {
                        T(y * rhs.z - z * rhs.y + x * rhs.w + w * rhs.x),
                        T(z * rhs.x - x * rhs.z + y * rhs.w + w * rhs.y),
                        T(x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z),
                        T(w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z)
                    };
                    x = r.x; y = r.y; z = r.z; w = r.w;
                    return r;
                }

                vec3<T> operator * (const vec3<T>& rhs) const {
                    quat<T> v = { rhs.x, rhs.y, rhs.z, T(0) };
                    quat<T> c = { T(-x), T(-y), T(-z), w };
                    quat<T> r = (*this * v) * c;
                    return { r.x, r.y, r.z };
                }

                quat<T> conjugate() const { return { T(-x), T(-y), T(-z), w }; }
                quat<T> inverse() const { T invMag = T(1) / mag(); return { T(-x * invMag), T(-y * invMag), T(-z * invMag), T(w * invMag) }; }

                T mag() const { return sqrt((x * x) + (y * y) + (z * z) + (w * w)); }
                T magSq() const { return (x * x) + (y * y) + (z * z) + (w * w); }

                T x, y, z, w;
        };

        template <typename T>
        class mat4 {
            public:
                mat4() :
                    x({ T(1), T(0), T(0), T(0) }),
                    y({ T(0), T(1), T(0), T(0) }),
                    z({ T(0), T(0), T(1), T(0) }),
                    w({ T(0), T(0), T(0), T(1) })
                { }
                mat4(T i) :
                    x({ i, T(0), T(0), T(0) }),
                    y({ T(0), i, T(0), T(0) }),
                    z({ T(0), T(0), i, T(0) }),
                    w({ T(0), T(0), T(0), i })
                { }
                mat4(
                    T xx, T xy, T xz, T xw,
                    T yx, T yy, T yz, T yw,
                    T zx, T zy, T zz, T zw,
                    T wx, T wy, T wz, T ww
                ) :
                    x({ xx, xy, xz, xw }),
                    y({ yx, yy, yz, yw }),
                    z({ zx, zy, zz, zw }),
                    w({ wx, wy, wz, ww })
                { }
                mat4(const mat3<T>& m) :
                    x({ m.x.x, m.x.y, m.x.z, T(0) }),
                    y({ m.y.x, m.y.y, m.y.z, T(0) }),
                    z({ m.z.x, m.z.y, m.z.z, T(0) }),
                    w({ T(0), T(0), T(0), T(1) })
                { }
                mat4(const vec4<T>& _x, const vec4<T>& _y, const vec4<T>& _z, const vec4<T>& _w) : x(_x), y(_y), z(_z), w(_w) { }

                mat4<T> operator + (const mat4<T>& rhs) const { return mat4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
                mat4<T> operator += (const mat4<T>& rhs) { return mat4<T>(x += rhs.x, y += rhs.y, z += rhs.z, w += rhs.w); }
                mat4<T> operator - (const mat4<T>& rhs) const { return mat4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
                mat4<T> operator -= (const mat4<T>& rhs) { return mat4<T>(x -= rhs.x, y -= rhs.y, z -= rhs.z, w -= rhs.w); }
                mat4<T> operator * (T rhs) const { return mat4<T>(x * rhs, y * rhs, z * rhs, w * rhs); }
                mat4<T> operator *= (T rhs) { return mat4<T>(x *= rhs, y *= rhs, z *= rhs, w *= rhs); }
                mat4<T> operator / (T rhs) const { return mat4<T>(x / rhs, y / rhs, z / rhs, w / rhs); }
                mat4<T> operator /= (T rhs) { return mat4<T>(x /= rhs, y /= rhs, z /= rhs, w /= rhs); }
                mat4<T> operator * (const mat4<T>& rhs) const {
                    return mat4<T>(
                        // row 1
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x) + (x.z * rhs.z.x) + (x.w * rhs.w.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y) + (x.z * rhs.z.y) + (x.w * rhs.w.y)),
                        T((x.x * rhs.x.z) + (x.y * rhs.y.z) + (x.z * rhs.z.z) + (x.w * rhs.w.z)),
                        T((x.x * rhs.x.w) + (x.y * rhs.y.w) + (x.z * rhs.z.w) + (x.w * rhs.w.w)),
                        // row 2
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x) + (y.z * rhs.z.x) + (y.w * rhs.w.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y) + (y.z * rhs.z.y) + (y.w * rhs.w.y)),
                        T((y.x * rhs.x.z) + (y.y * rhs.y.z) + (y.z * rhs.z.z) + (y.w * rhs.w.z)),
                        T((y.x * rhs.x.w) + (y.y * rhs.y.w) + (y.z * rhs.z.w) + (y.w * rhs.w.w)),
                        // row 3
                        T((z.x * rhs.x.x) + (z.y * rhs.y.x) + (z.z * rhs.z.x) + (z.w * rhs.w.x)),
                        T((z.x * rhs.x.y) + (z.y * rhs.y.y) + (z.z * rhs.z.y) + (z.w * rhs.w.y)),
                        T((z.x * rhs.x.z) + (z.y * rhs.y.z) + (z.z * rhs.z.z) + (z.w * rhs.w.z)),
                        T((z.x * rhs.x.w) + (z.y * rhs.y.w) + (z.z * rhs.z.w) + (z.w * rhs.w.w)),
                        // row 4
                        T((w.x * rhs.x.x) + (w.y * rhs.y.x) + (w.z * rhs.z.x) + (w.w * rhs.w.x)),
                        T((w.x * rhs.x.y) + (w.y * rhs.y.y) + (w.z * rhs.z.y) + (w.w * rhs.w.y)),
                        T((w.x * rhs.x.z) + (w.y * rhs.y.z) + (w.z * rhs.z.z) + (w.w * rhs.w.z)),
                        T((w.x * rhs.x.w) + (w.y * rhs.y.w) + (w.z * rhs.z.w) + (w.w * rhs.w.w))
                    );
                }
                mat4<T> operator *= (const mat4<T>& rhs) {
                    x = {
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x) + (x.z * rhs.z.x) + (x.w * rhs.w.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y) + (x.z * rhs.z.y) + (x.w * rhs.w.y)),
                        T((x.x * rhs.x.z) + (x.y * rhs.y.z) + (x.z * rhs.z.z) + (x.w * rhs.w.z)),
                        T((x.x * rhs.x.w) + (x.y * rhs.y.w) + (x.z * rhs.z.w) + (x.w * rhs.w.w))
                    };
                    y = {
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x) + (y.z * rhs.z.x) + (y.w * rhs.w.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y) + (y.z * rhs.z.y) + (y.w * rhs.w.y)),
                        T((y.x * rhs.x.z) + (y.y * rhs.y.z) + (y.z * rhs.z.z) + (y.w * rhs.w.z)),
                        T((y.x * rhs.x.w) + (y.y * rhs.y.w) + (y.z * rhs.z.w) + (y.w * rhs.w.w))
                    };
                    z = {
                        T((z.x * rhs.x.x) + (z.y * rhs.y.x) + (z.z * rhs.z.x) + (z.w * rhs.w.x)),
                        T((z.x * rhs.x.y) + (z.y * rhs.y.y) + (z.z * rhs.z.y) + (z.w * rhs.w.y)),
                        T((z.x * rhs.x.z) + (z.y * rhs.y.z) + (z.z * rhs.z.z) + (z.w * rhs.w.z)),
                        T((z.x * rhs.x.w) + (z.y * rhs.y.w) + (z.z * rhs.z.w) + (z.w * rhs.w.w))
                    };
                    w = {
                        T((w.x * rhs.x.x) + (w.y * rhs.y.x) + (w.z * rhs.z.x) + (w.w * rhs.w.x)),
                        T((w.x * rhs.x.y) + (w.y * rhs.y.y) + (w.z * rhs.z.y) + (w.w * rhs.w.y)),
                        T((w.x * rhs.x.z) + (w.y * rhs.y.z) + (w.z * rhs.z.z) + (w.w * rhs.w.z)),
                        T((w.x * rhs.x.w) + (w.y * rhs.y.w) + (w.z * rhs.z.w) + (w.w * rhs.w.w))
                    };
                    return *this;
                }
                vec2<T> operator * (const vec2<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y + x.z + x.w),
                        T(y.x * rhs.x + y.y * rhs.y + y.z + y.w)
                    };
                }
                vec3<T> operator * (const vec3<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y + x.z * rhs.z + x.w),
                        T(y.x * rhs.x + y.y * rhs.y + y.z * rhs.z + y.w),
                        T(z.x * rhs.x + z.y * rhs.y + z.z * rhs.z + z.w)
                    };
                }
                vec4<T> operator * (const vec4<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y + x.z * rhs.z + x.w * rhs.w),
                        T(y.x * rhs.x + y.y * rhs.y + y.z * rhs.z + y.w * rhs.w),
                        T(z.x * rhs.x + z.y * rhs.y + z.z * rhs.z + z.w * rhs.w),
                        T(w.x * rhs.x + w.y * rhs.y + w.z * rhs.z + w.w * rhs.w)
                    };
                }

                vec4<T>& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 3
                    return (&x)[idx];
                }

                vec3<T> col(u8 idx) const {
                    if (idx == 0) return vec4<T>(x.x, y.x, z.x, w.x);
                    else if (idx == 1) return vec4<T>(x.y, y.y, z.y, w.y);
                    else if (idx == 2) return vec4<T>(x.z, y.z, z.z, w.z);
                    else if (idx == 3) return vec4<T>(x.w, y.w, z.w, w.w);
                    else {
                        // todo: runtime exception if idx > 3
                    }
                }
                static mat4<T> rotationX(T deg) {
                    T rad = d2r(deg);
                    T cosTh = cos(rad);
                    T sinTh = sin(rad);
                    return mat4<T>(
                        T(1), T(0), T(0), T(0),
                        T(0), cosTh, -sinTh, T(0),
                        T(0), sinTh, cosTh, T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> rotationY(T deg) {
                    T rad = d2r(deg);
                    T cosTh = cos(rad);
                    T sinTh = sin(rad);
                    return mat4<T>(
                        cosTh, T(0), sinTh, T(0),
                        T(0), T(1), T(0), T(0),
                        -sinTh, T(0), cosTh, T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> rotationZ(T deg) {
                    T rad = d2r(deg);
                    T cosTh = cos(rad);
                    T sinTh = sin(rad);
                    return mat4<T>(
                        cosTh, -sinTh, T(0), T(0),
                        sinTh, cosTh, T(0), T(0),
                        T(0), T(0), T(1), T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> rotationQuat(const quat<T>& quat) {
                    vec4<T> sq(quat.x * quat.x, quat.y * quat.y, quat.z * quat.z, quat.w * quat.w);
                    T invS = T(1) / (sq.x + sq.y + sq.z + sq.w);

                    T t1 = quat.x * quat.y;
                    T t2 = quat.z * quat.w;
                    T t3 = quat.x * quat.z;
                    T t4 = quat.y * quat.w;
                    T t5 = quat.y * quat.z;
                    T t6 = quat.x * quat.w;

                    return mat4<T>(
                        (sq.x - sq.y - sq.z + sq.w) * invS, T(2) * (t1 - t2) * invS            , T(2) * (t3 + t4) * invS            , T(0),
                        T(2) * (t1 + t2) * invS           , (-sq.x + sq.y - sq.z + sq.w) * invS, T(2) * (t5 - t6) * invS            , T(0),
                        T(2) * (t3 - t4) * invS           , T(2) * (t5 + t6) * invS            , (-sq.x - sq.y + sq.z + sq.w) * invS, T(0),
                        T(0), T(0), T(0), T(1)
                        );
                }
                static mat4<T> rotation(const vec3<T>& axis, T angleDeg) {
                    return rotationQuat(quat<T>(axis, angleDeg));
                }
                static mat4<T> rotationEuler(const vec3<T>& degXYZ) {
                    vec3<T> rad(d2r(degXYZ.x), d2r(degXYZ.y), d2r(degXYZ.z));
                    T cx = cos(rad.x);
                    T sx = sin(rad.x);
                    T cy = cos(rad.y);
                    T sy = sin(rad.y);
                    T cz = cos(rad.z);
                    T sz = sin(rad.z);

                    return mat4<T>(
                        cy * cz , sy * sx - cy * sz * cx, cy * sz * sx + sy * cx , T(0),
                        sz      , cz * cx               , -cz * sx               , T(0),
                        -sy * cz, sy * sz * cx + cy * sx, -sy * sz * sx + cy * cx, T(0),
                        T(0)    , T(0)                  , T(0)                   , T(1)
                    );
                }
                /*

                */
                static mat4<T> scale(T amount) {
                    return mat4<T>(
                        amount, T(0), T(0), T(0),
                        T(0), amount, T(0), T(0),
                        T(0), T(0), amount, T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> scale(T x, T y, T z) {
                    return mat4<T>(
                        x, T(0), T(0), T(0),
                        T(0), y, T(0), T(0),
                        T(0), T(0), z, T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> scale(const vec3<T>& amount) {
                    return mat4<T>(
                        amount.x, T(0), T(0), T(0),
                        T(0), amount.y, T(0), T(0),
                        T(0), T(0), amount.z, T(0),
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> translate3D(const vec3<T> trans) {
                    return mat4<T>(
                        T(1), T(0), T(0), trans.x,
                        T(0), T(1), T(0), trans.y,
                        T(0), T(0), T(1), trans.z,
                        T(0), T(0), T(0), T(1)
                    );
                }
                static mat4<T> compose3D(const vec3<T>& eulerDeg, const vec3<T>& scale, const vec3<T>& trans) {
                    return translate3D(trans) * rotationEuler(eulerDeg) * mat4<T>::scale(scale.x, scale.y, T(1));
                }
                static mat4<T> compose3D(const vec3<T>& axis, T angleDeg, const vec3<T>& scale, const vec3<T>& trans) {
                    return translate3D(trans) * rotation(axis, angleDeg) * mat4<T>::scale(scale.x, scale.y, T(1));
                }
                static mat4<T> compose3D(const quat<T>& rot, const vec3<T>& scale, const vec3<T>& trans) {
                    return translate3D(trans) * rotationQuat(rot) * mat4<T>::scale(scale.x, scale.y, T(1));
                }

                vec4<T> x, y, z, w;
        };

        template <typename T>
        vec4<T> min(const vec4<T>& x, const vec4<T>& y) { return { min(x.x, y.x), min(x.y, y.y), min(x.z, y.z), min(x.w, y.w) }; }

        template <typename T>
        vec4<T> max(const vec4<T>& x, const vec4<T>& y) { return { max(x.x, y.x), max(x.y, y.y), min(x.z, y.z), min(x.w, y.w) }; }

        template <typename T>
        vec4<T> random(const vec4<T>& x, const vec4<T>& y) { return { random(x.x, y.x), random(x.y, y.y), random(x.z, y.z), random(x.w, y.w) }; }
        
        void bind_vec4(script_module* mod);
    };
};