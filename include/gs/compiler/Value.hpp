#pragma once
#include <gs/compiler/Value.h>
#include <utils/Array.hpp>

#include <xtr1common>

namespace gs {
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
                if constexpr (std::is_same_v<T, ffi::Function*>) return m_imm.fn;
                else if constexpr (std::is_same_v<T, Module*>) return m_imm.mod;
                return m_imm.f;
            }
        }
    };
};