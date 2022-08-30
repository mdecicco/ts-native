#include <gs/compiler/FunctionDef.h>
#include <gs/compiler/Compiler.h>
#include <gs/common/Context.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>

namespace gs {
    namespace compiler {
        template <typename T>
        Value& FunctionDef::val(const utils::String& name) {
            return val(name, m_comp->getContext()->getTypes()->getType<T>());
        }

        template <typename T>
        Value FunctionDef::val() {
            return val(m_comp->getContext()->getTypes()->getType<T>());
        }

        template <typename T>
        std::enable_if_t<is_imm_v<T>, Value>
        FunctionDef::imm(T value) {
            if constexpr (std::is_same_v<T, ffi::Function*>) {
                return Value(m_comp, value);
            } else if constexpr (std::is_unsigned_v<T>) {
                return Value(m_comp, (u64)value);
            } else if constexpr (std::is_floating_point_v<T>) {
                return Value(m_comp, (f64)value);
            } else {
                return Value(m_comp, (i64)value);
            }
        }
    };
};