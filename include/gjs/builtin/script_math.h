#pragma once
#include <stdlib.h>
#include <gjs/common/types.h>

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

        void bind_math(script_context* ctx);
    };
};