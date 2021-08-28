#pragma once
#include <gjs/builtin/script_math.h>

namespace gjs {
    class script_module;

    namespace math {
        template <typename T>
        class vec3 {
            public:
                vec3() : x(T(0)), y(T(0)), z(T(0)) {}
                vec3(T v) : x(v), y(v), z(v) {}
                vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
                vec3(const vec2<T>& xy) : x(xy.x), y(xy.y), z(T(0)) {}
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
                vec3<T> operator - () const { return { T(-x), T(-y), T(-z) }; }
                T& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 2
                    return (&x)[idx];
                }

                T mag() const { return (T)sqrt((x * x) + (y * y) + (z * z)); }
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
        class mat3 {
            public:
                mat3() :
                    x({ T(1), T(0), T(0) }),
                    y({ T(0), T(1), T(0) }),
                    z({ T(0), T(0), T(1) })
                { }
                mat3(T i) :
                    x({ i, T(0), T(0) }),
                    y({ T(0), i, T(0) }),
                    z({ T(0), T(0), i })
                { }
                mat3(
                    T xx, T xy, T xz,
                    T yx, T yy, T yz,
                    T zx, T zy, T zz
                ) :
                    x({ xx, xy, xz }),
                    y({ yx, yy, yz }),
                    z({ zx, zy, zz })
                { }
                mat3(const vec3<T>& _x, const vec3<T>& _y, const vec3<T>& _z) : x(_x), y(_y), z(_z) { }

                mat3<T> operator + (const mat3<T>& rhs) const { return mat3<T>(x + rhs.x, y + rhs.y, z + rhs.z); }
                mat3<T> operator += (const mat3<T>& rhs) { return mat3<T>(x += rhs.x, y += rhs.y, z += rhs.z); }
                mat3<T> operator - (const mat3<T>& rhs) const { return mat3<T>(x - rhs.x, y - rhs.y, z - rhs.z); }
                mat3<T> operator -= (const mat3<T>& rhs) { return mat3<T>(x -= rhs.x, y -= rhs.y, z -= rhs.z); }
                mat3<T> operator * (T rhs) const { return mat3<T>(x * rhs, y * rhs, z * rhs); }
                mat3<T> operator *= (T rhs) { return mat3<T>(x *= rhs, y *= rhs, z *= rhs); }
                mat3<T> operator / (T rhs) const { return mat3<T>(x / rhs, y / rhs, z / rhs); }
                mat3<T> operator /= (T rhs) { return mat3<T>(x /= rhs, y /= rhs, z /= rhs); }
                mat3<T> operator * (const mat3<T>& rhs) const {
                    return mat3<T>(
                        // row 1
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x) + (x.z * rhs.z.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y) + (x.z * rhs.z.y)),
                        T((x.x * rhs.x.z) + (x.y * rhs.y.z) + (x.z * rhs.z.z)),
                        // row 2
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x) + (y.z * rhs.z.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y) + (y.z * rhs.z.y)),
                        T((y.x * rhs.x.z) + (y.y * rhs.y.z) + (y.z * rhs.z.z)),
                        // row 3
                        T((z.x * rhs.x.x) + (z.y * rhs.y.x) + (z.z * rhs.z.x)),
                        T((z.x * rhs.x.y) + (z.y * rhs.y.y) + (z.z * rhs.z.y)),
                        T((z.x * rhs.x.z) + (z.y * rhs.y.z) + (z.z * rhs.z.z))
                    );
                }
                mat3<T> operator *= (const mat3<T>& rhs) {
                    x = {
                        T((x.x * rhs.x.x) + (x.y * rhs.y.x) + (x.z * rhs.z.x)),
                        T((x.x * rhs.x.y) + (x.y * rhs.y.y) + (x.z * rhs.z.y)),
                        T((x.x * rhs.x.z) + (x.y * rhs.y.z) + (x.z * rhs.z.z))
                    };
                    y = {
                        T((y.x * rhs.x.x) + (y.y * rhs.y.x) + (y.z * rhs.z.x)),
                        T((y.x * rhs.x.y) + (y.y * rhs.y.y) + (y.z * rhs.z.y)),
                        T((y.x * rhs.x.z) + (y.y * rhs.y.z) + (y.z * rhs.z.z))
                    };
                    z = {
                        T((z.x * rhs.x.x) + (z.y * rhs.y.x) + (z.z * rhs.z.x)),
                        T((z.x * rhs.x.y) + (z.y * rhs.y.y) + (z.z * rhs.z.y)),
                        T((z.x * rhs.x.z) + (z.y * rhs.y.z) + (z.z * rhs.z.z))
                    };
                    return *this;
                }
                vec2<T> operator * (const vec2<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y + x.z),
                        T(y.x * rhs.x + y.y * rhs.y + y.z)
                    };
                }
                vec3<T> operator * (const vec3<T>& rhs) const {
                    return {
                        T(x.x * rhs.x + x.y * rhs.y + x.z * rhs.z),
                        T(y.x * rhs.x + y.y * rhs.y + y.z * rhs.z),
                        T(z.x * rhs.x + z.y * rhs.y + z.z * rhs.z)
                    };
                }

                vec3<T>& operator [] (u8 idx) {
                    // todo: runtime exception if idx > 2
                    return (&x)[idx];
                }

                vec3<T> col(u8 idx) const {
                    if (idx == 0) return vec3<T>(x.x, y.x, z.x);
                    else if (idx == 1) return vec3<T>(x.y, y.y, z.y);
                    else if (idx == 2) return vec3<T>(x.z, y.z, z.z);
                    else {
                        // todo: runtime exception if idx > 2
                    }
                    return vec3<T>(x.x, y.x, z.x);
                }

                static mat3<T> rotationX(T deg) {
                    T rad = d2r(deg);
                    T cosTh = (T)cos(rad);
                    T sinTh = (T)sin(rad);
                    return mat3<T>(
                        T(1), T(0), T(0),
                        T(0), cosTh, -sinTh,
                        T(0), sinTh, cosTh
                    );
                }
                static mat3<T> rotationY(T deg) {
                    T rad = d2r(deg);
                    T cosTh = (T)cos(rad);
                    T sinTh = (T)sin(rad);
                    return mat3<T>(
                        cosTh, T(0), sinTh,
                        T(0), T(1), T(0),
                        -sinTh, T(0), cosTh
                    );
                }
                static mat3<T> rotationZ(T deg) {
                    T rad = d2r(deg);
                    T cosTh = (T)cos(rad);
                    T sinTh = (T)sin(rad);
                    return mat3<T>(
                        cosTh, -sinTh, T(0),
                        sinTh, cosTh, T(0),
                        T(0), T(0), T(1)
                    );
                }
                static mat3<T> rotationEuler(const vec3<T>& degXYZ) {
                    vec3<T> rad(d2r(degXYZ.x), d2r(degXYZ.y), d2r(degXYZ.z));
                    vec3<T> cosTh((T)cos(rad.x), (T)cos(rad.y), (T)cos(rad.z));
                    vec3<T> sinTh((T)sin(rad.x), (T)sin(rad.y), (T)sin(rad.z));

                    return mat3<T>(
                         cosTh.y * cosTh.z                                  , cosTh.y * sinTh.z                                 , -sinTh.y         ,
                        (sinTh.x * sinTh.y * cosTh.z) - (cosTh.x * sinTh.z), (sinTh.x * sinTh.y * sinTh.z) + (cosTh.x * cosTh.z),  sinTh.x * cosTh.y,
                        (cosTh.x * sinTh.y * cosTh.z) + (sinTh.x * sinTh.z), (cosTh.x * sinTh.y * sinTh.z) - (sinTh.x * cosTh.z),  cosTh.x * cosTh.y
                    );
                }
                static mat3<T> scale(T amount) {
                    return mat3<T>(
                        amount, T(0), T(0),
                        T(0), amount, T(0),
                        T(0), T(0), amount
                    );
                }
                static mat3<T> scale(T x, T y, T z) {
                    return mat3<T>(
                        x, T(0), T(0),
                        T(0), y, T(0),
                        T(0), T(0), z
                    );
                }
                static mat3<T> scale(const vec3<T>& amount) {
                    return mat3<T>(
                        amount.x, T(0), T(0),
                        T(0), amount.y, T(0),
                        T(0), T(0), amount.z
                    );
                }
                static mat3<T> translate2D(const vec2<T>& trans) {
                    return mat3<T>(
                        T(1), T(0), trans.x,
                        T(0), T(1), trans.y,
                        T(0), T(0), T(1)
                    );
                }
                static mat3<T> compose2D(T rotDeg, const vec2<T>& scale, const vec2<T>& trans) {
                    return translate2D(trans) * rotationZ(rotDeg) * mat3<T>::scale(scale.x, scale.y, T(1));
                }

                vec3<T> x, y, z;
        };

        template <typename T>
        vec3<T> min(const vec3<T>& x, const vec3<T>& y) { return { min(x.x, y.x), min(x.y, y.y), min(x.z, y.z) }; }

        template <typename T>
        vec3<T> max(const vec3<T>& x, const vec3<T>& y) { return { max(x.x, y.x), max(x.y, y.y), min(x.z, y.z) }; }

        template <typename T>
        vec3<T> random(const vec3<T>& x, const vec3<T>& y) { return { random(x.x, y.x), random(x.y, y.y), random(x.z, y.z) }; }

        void bind_vec3(script_module* mod);
    };
};