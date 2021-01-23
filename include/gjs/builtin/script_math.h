#pragma once
#include <stdlib.h>

namespace gjs {
    class script_context;

    namespace math {
        template <typename T>
        T min(T x, T y) { return x < y ? x : y; }
        template <typename T>
        T max(T x, T y) { return x > y ? x : y; }
        template <typename T>
        T random(T x, T y) { return x + ((T(rand()) / T(RAND_MAX)) * (y - x)); }
        template <typename T>
        T sign(T x) { return x < 0 ? T(-1) : T(1); }
        template <typename T>
        T r2d(T x) { return x * (T(180.0) / T(3.14159265358979323846)); }
        template <typename T>
        T d2r(T x) { return x * (T(3.14159265358979323846) / T(180.0)); }
        
        template <typename T>
        class vec2 {
            public:
                vec2() : x(T(0)), y(T(0)) {}
                vec2(T v) : x(v), y(v) {}
                vec2(T _x, T _y) : x(_x), y(_y) {}

                vec2<T> operator + (const vec2<T>& rhs) const { return { T(x + rhs.x), T(y + rhs.y) }; }
                vec2<T> operator += (const vec2<T>& rhs) { return { T(x += rhs.x), T(y += rhs.y) }; }
                vec2<T> operator - (const vec2<T>& rhs) const { return { T(x - rhs.x), T(y - rhs.y) }; }
                vec2<T> operator -= (const vec2<T>& rhs) { return { T(x -= rhs.x), T(y -= rhs.y) }; }
                vec2<T> operator * (const vec2<T>& rhs) const { return { T(x * rhs.x), T(y * rhs.y) }; }
                vec2<T> operator *= (const vec2<T>& rhs) { return { T(x *= rhs.x), T(y *= rhs.y) }; }
                vec2<T> operator / (const vec2<T>& rhs) const { return { T(x / rhs.x), T(y / rhs.y) }; }
                vec2<T> operator /= (const vec2<T>& rhs) { return { T(x /= rhs.x), T(y /= rhs.y) }; }
                vec2<T> operator + (T rhs) const { return { T(x + rhs), T(y + rhs) }; }
                vec2<T> operator += (T rhs) { return { T(x += rhs), T(y += rhs) }; }
                vec2<T> operator - (T rhs) const { return { T(x - rhs), T(y - rhs) }; }
                vec2<T> operator -= (T rhs) { return { T(x -= rhs), T(y -= rhs) }; }
                vec2<T> operator * (T rhs) const { return { T(x * rhs), T(y * rhs) }; }
                vec2<T> operator *= (T rhs) { return { T(x *= rhs), T(y *= rhs) }; }
                vec2<T> operator / (T rhs) const { return { T(x / rhs), T(y / rhs) }; }
                vec2<T> operator /= (T rhs) { return { T(x /= rhs), T(y /= rhs) }; }

                T mag() const { return sqrt((x * x) + (y * y)); }
                T magSq() const { return (x * x) + (y * y); }
                T distance(const vec2<T>& v) const { return (*this - v).mag(); }
                T distanceSq(const vec2<T>& v) const { return (*this - v).magSq(); }
                T dot(const vec2<T>& v) const { return (x * v.x) + (y * v.y); }

                vec2<T> normalized() const {
                    T invMag = T(1.0) / mag();
                    return { T(x * invMag), T(y * invMag) };
                }
                void normalize() {
                    T invMag = T(1.0) / mag();
                    x *= invMag; y *= invMag;
                }

                static vec2<T> fromAngleMag_r(T angleRadians, T mag) { return { T(cos(angleRadians) * mag), T(sin(angleRadians) * mag) }; }
                static vec2<T> fromAngleMag(T angleDegrees, T mag) {
                    T r = d2r(angleDegrees);
                    return { T(cos(r) * mag), T(sin(r) * mag) };
                }

                T x, y;
        };

        template <typename T>
        class vec3 {
            public:
                vec3() : x(T(0)), y(T(0)), z(T(0)) {}
                vec3(T v) : x(v), y(v), z(v) {}
                vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
                vec3(const vec2<T>& xy, T _z) : x(xy.x), y(xy.y), z(_z) {}
                vec3(T _x, const vec2<T>& yz) : x(_x), y(yz.x), z(yz.y) {}

                vec3<T> operator + (const vec3<T>& rhs) const { return { T(x + rhs.x), T(y + rhs.y), T(z + rhs.z) }; }
                vec3<T> operator += (const vec3<T>& rhs) { return { T(x += rhs.x), T(y += rhs.y), T(z += rhs.z) }; }
                vec3<T> operator - (const vec3<T>& rhs) const { return { T(x - rhs.x), T(y - rhs.y), T(z - rhs.z) }; }
                vec3<T> operator -= (const vec3<T>& rhs) { return { T(x -= rhs.x), T(y -= rhs.y), T(z -= rhs.z) }; }
                vec3<T> operator * (const vec3<T>& rhs) const { return { T(x * rhs.x), T(y * rhs.y), T(z * rhs.z) }; }
                vec3<T> operator *= (const vec3<T>& rhs) { return { T(x *= rhs.x), T(y *= rhs.y), T(z *= rhs.z) }; }
                vec3<T> operator / (const vec3<T>& rhs) const { return { T(x / rhs.x), T(y / rhs.y), T(y / rhs.z) }; }
                vec3<T> operator /= (const vec3<T>& rhs) { return { T(x /= rhs.x), T(y /= rhs.y), T(y /= rhs.z) }; }
                vec3<T> operator + (T rhs) const { return { T(x + rhs), T(y + rhs), T(z + rhs) }; }
                vec3<T> operator += (T rhs) { return { T(x += rhs), T(y += rhs), T(z += rhs) }; }
                vec3<T> operator - (T rhs) const { return { T(x - rhs), T(y - rhs), T(z - rhs) }; }
                vec3<T> operator -= (T rhs) { return { T(x -= rhs), T(y -= rhs), T(z -= rhs) }; }
                vec3<T> operator * (T rhs) const { return { T(x * rhs), T(y * rhs), T(z * rhs) }; }
                vec3<T> operator *= (T rhs) { return { T(x *= rhs), T(y *= rhs), T(z *= rhs) }; }
                vec3<T> operator / (T rhs) const { return { T(x / rhs), T(y / rhs), T(z / rhs) }; }
                vec3<T> operator /= (T rhs) { return { T(x /= rhs), T(y /= rhs), T(z /= rhs) }; }

                T mag() const { return sqrt((x * x) + (y * y) + (z * z)); }
                T magSq() const { return (x * x) + (y * y) + (z * z); }
                T distance(const vec3<T>& v) const { return (*this - v).mag(); }
                T distanceSq(const vec3<T>& v) const { return (*this - v).magSq(); }
                T dot(const vec3<T>& v) const { return (x * v.x) + (y * v.y) + (z * v.z); }
                vec3<T> cross(const vec3<T>& v) const { return { T((y * v.z) - (z * v.y)), T((z * v.x) - (x * v.z)), T((x * v.y) - (y * v.x)) }; }
                vec3<T> normalized() const {
                    T invMag = T(1.0) / mag();
                    return { T(x * invMag), T(y * invMag), T(z * invMag) };
                }
                void normalize() {
                    T invMag = T(1.0) / mag();
                    x *= invMag; y *= invMag; z *= invMag;
                }

                vec2<T> xx() const { return { x, x }; }
                vec2<T> xy() const { return { x, y }; }
                vec2<T> xz() const { return { x, z }; }
                vec2<T> yx() const { return { y, x }; }
                vec2<T> yy() const { return { y, y }; }
                vec2<T> yz() const { return { y, z }; }
                vec2<T> zx() const { return { z, x }; }
                vec2<T> zy() const { return { z, y }; }
                vec2<T> zz() const { return { z, z }; }

                T x, y, z;
        };

        template <typename T>
        class vec4 {
            public:
                vec4() : x(T(0)), y(T(0)), z(T(0)), w(T(0)) {}
                vec4(T v) : x(v), y(v), z(v), w(v) {}
                vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
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
        vec2<T> min(const vec2<T>& x, const vec2<T>& y) { return { min(x.x, y.x), min(x.y, y.y) }; }

        template <typename T>
        vec2<T> max(const vec2<T>& x, const vec2<T>& y) { return { max(x.x, y.x), max(x.y, y.y) }; }

        template <typename T>
        vec3<T> min(const vec3<T>& x, const vec3<T>& y) { return { min(x.x, y.x), min(x.y, y.y), min(x.z, y.z) }; }

        template <typename T>
        vec3<T> max(const vec3<T>& x, const vec3<T>& y) { return { max(x.x, y.x), max(x.y, y.y), min(x.z, y.z) }; }

        template <typename T>
        vec4<T> min(const vec4<T>& x, const vec4<T>& y) { return { min(x.x, y.x), min(x.y, y.y), min(x.z, y.z), min(x.w, y.w) }; }

        template <typename T>
        vec4<T> max(const vec4<T>& x, const vec4<T>& y) { return { max(x.x, y.x), max(x.y, y.y), min(x.z, y.z), min(x.w, y.w) }; }

        template <typename T>
        vec2<T> random(const vec2<T>& x, const vec2<T>& y) { return { random(x.x, y.x), random(x.y, y.y) }; }

        template <typename T>
        vec3<T> random(const vec3<T>& x, const vec3<T>& y) { return { random(x.x, y.x), random(x.y, y.y), random(x.z, y.z) }; }

        template <typename T>
        vec4<T> random(const vec4<T>& x, const vec4<T>& y) { return { random(x.x, y.x), random(x.y, y.y), random(x.z, y.z), random(x.w, y.w) }; }

        void bind_math(script_context* ctx);
    };
};