#pragma once
#include <tsn/compiler/Value.h>
#include <utils/Array.hpp>

#include <type_traits>

namespace tsn {
    namespace ffi {
        class Function;
    };

    namespace compiler {
        template <typename T>
        std::enable_if_t<is_imm_v<T>, T> Value::getImm() const {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_unsigned_v<T>) return T(m_imm.u);
                return T(m_imm.i);
            } else {
                if constexpr (std::is_floating_point_v<T>) return T(m_imm.f);
                else if constexpr (std::is_same_v<T, FunctionDef*>) return m_imm.fn;
                else if constexpr (std::is_same_v<T, Module*>) return m_imm.mod;
            }
        }

        template <typename T>
        void Value::setImm(T val) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_unsigned_v<T>) m_imm.u = val;
                else m_imm.i = val;
            } else {
                if constexpr (std::is_floating_point_v<T>) m_imm.f = val;
                else if constexpr (std::is_same_v<T, FunctionDef*>) m_imm.fn = val;
                else if constexpr (std::is_same_v<T, Module*>) m_imm.mod = val;
            }
        }
    };
};