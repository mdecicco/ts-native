#pragma once
#include <gjs/builtin/script_math.h>

namespace gjs {
    class script_module;

    namespace math {
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
                vec2<T> operator - () const { return { T(-x), T(-y) }; }
                T& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 1
                    return (&x)[idx];
                }

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
        class mat2 {
            public:
                mat2() :
                    x({ T(1), T(0) }),
                    y({ T(0), T(1) })
                { }
                mat2(T i) :
                    x({ i, T(0) }),
                    y({ T(0), i })
                { }
                mat2(
                    T xx, T xy,
                    T yx, T yy
                ) :
                    x({ xx, xy }),
                    y({ yx, yy })
                { }
                mat2(const vec2<T>& _x, const vec2<T>& _y) : x(_x), y(_y) { }

                mat2<T> operator + (const mat2<T>& rhs) const { return mat2<T>(x + rhs.x, y + rhs.y); }
                mat2<T> operator += (const mat2<T>& rhs) { return mat2<T>(x += rhs.x, y += rhs.y); }
                mat2<T> operator - (const mat2<T>& rhs) const { return mat2<T>(x - rhs.x, y - rhs.y); }
                mat2<T> operator -= (const mat2<T>& rhs) { return mat2<T>(x -= rhs.x, y -= rhs.y); }
                mat2<T> operator * (T rhs) const { return mat2<T>(x * rhs, y * rhs); }
                mat2<T> operator *= (T rhs) { return mat2<T>(x *= rhs, y *= rhs); }
                mat2<T> operator / (T rhs) const { return mat2<T>(x / rhs, y / rhs); }
                mat2<T> operator /= (T rhs) { return mat2<T>(x /= rhs, y /= rhs); }
                mat2<T> operator * (const mat2<T>& rhs) const {
                    return mat2<T>(
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y)),
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y))
                    );
                }
                mat2<T> operator *= (const mat2<T>& rhs) {
                    x = {
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y))
                    };
                    y = {
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y))
                    };
                    return *this;
                }
                vec2<T> operator * (const vec2<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y),
                        T(y.x * rhs.x + y.y * rhs.y)
                    };
                }

                vec2<T>& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 1
                    return (&x)[idx];
                }

                vec2<T> col(u8 idx) const {
                    if (idx == 0) return vec2<T>(x.x, y.x);
                    else if (idx == 1) return vec2<T>(x.y, y.y);
                    else {
                        // todo: runtime exception if idx > 1
                    }
                    return vec2<T>(x.x, y.x);
                }

                static mat2<T> rotation(T deg) {
                    T rad = d2r(deg);
                    T cosTh = cos(rad);
                    T sinTh = sin(rad);
                    return mat2<T>(
                        cosTh, -sinTh,
                        sinTh, cosTh
                        );
                }
                static mat2<T> scale(T amount) {
                    return mat2<T>(
                        amount, T(0),
                        T(0), amount
                    );
                }
                static mat2<T> scale(T x, T y) {
                    return mat2<T>(
                        x, T(0),
                        T(0), y
                    );
                }
                static mat2<T> scale(const vec2<T>& amount) {
                    return mat2<T>(
                        amount.x, T(0),
                        T(0), amount.y
                    );
                }

                vec2<T> x, y;
        };

        template <typename T>
        vec2<T> min(const vec2<T>& x, const vec2<T>& y) { return { min(x.x, y.x), min(x.y, y.y) }; }

        template <typename T>
        vec2<T> max(const vec2<T>& x, const vec2<T>& y) { return { max(x.x, y.x), max(x.y, y.y) }; }

        template <typename T>
        vec2<T> random(const vec2<T>& x, const vec2<T>& y) { return { random(x.x, y.x), random(x.y, y.y) }; }

        void bind_vec2(script_module* mod);
    };
};