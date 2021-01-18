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
        T random() { return rand(); }
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
                vec2(T _x, T _y) : x(_x), y(_y) {}

                vec2<T> operator + (const vec2<T>& rhs) const { return vec2<T>(x + rhs.x, y + rhs.y); }
                vec2<T> operator += (const vec2<T>& rhs) { return vec2<T>(x += rhs.x, y += rhs.y); }
                vec2<T> operator - (const vec2<T>& rhs) const { return vec2<T>(x - rhs.x, y - rhs.y); }
                vec2<T> operator -= (const vec2<T>& rhs) { return vec2<T>(x -= rhs.x, y -= rhs.y); }
                vec2<T> operator * (const vec2<T>& rhs) const { return vec2<T>(x * rhs.x, y * rhs.y); }
                vec2<T> operator *= (const vec2<T>& rhs) { return vec2<T>(x *= rhs.x, y *= rhs.y); }
                vec2<T> operator / (const vec2<T>& rhs) const { return vec2<T>(x / rhs.x, y / rhs.y); }
                vec2<T> operator /= (const vec2<T>& rhs) { return vec2<T>(x /= rhs.x, y /= rhs.y); }
                vec2<T> operator + (T rhs) const { return vec2<T>(x + rhs, y + rhs); }
                vec2<T> operator += (T rhs) { return vec2<T>(x += rhs, y += rhs); }
                vec2<T> operator - (T rhs) const { return vec2<T>(x - rhs, y - rhs); }
                vec2<T> operator -= (T rhs) { return vec2<T>(x -= rhs, y -= rhs); }
                vec2<T> operator * (T rhs) const { return vec2<T>(x * rhs, y * rhs); }
                vec2<T> operator *= (T rhs) { return vec2<T>(x *= rhs, y *= rhs); }
                vec2<T> operator / (T rhs) const { return vec2<T>(x / rhs, y / rhs); }
                vec2<T> operator /= (T rhs) { return vec2<T>(x /= rhs, y /= rhs); }

                T mag() const { return sqrt((x * x) + (y * y)); }
                T magSq() const { return (x * x) + (y * y); }

                static vec2<T> fromAngleMag_r(T angleRadians, T mag) { return vec2<T>(cos(angleRadians) * mag, sin(angleRadians) * mag); }
                static vec2<T> fromAngleMag(T angleDegrees, T mag) {
                    T r = d2r(angleDegrees);
                    return vec2<T>(cos(r) * mag, sin(r) * mag);
                }

                T x, y;
        };

        void bind_math(script_context* ctx);
    };
};