#pragma once
#include <gs/compiler/Value.h>
#include <utils/Array.hpp>

#include <xtr1common>

namespace gs {
    namespace compiler {
        template <typename T>
        std::enable_if_t<is_imm_v<T>, T> Value::getImm() const {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_unsigned_v<T>) return m_imm.u;
                return m_imm.i;
            } else {
                if (std::is_same_v<T, ffi::Function*>) return m_imm.fn;
                return m_imm.f;
            }
        }
    };
};