#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/common/Context.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>

namespace tsn {
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
            if constexpr (std::is_same_v<T, FunctionDef*>) {
                return Value(this, value);
            } else if constexpr (std::is_same_v<T, Module*>) {
                return Value(this, value);
            } else if constexpr (std::is_unsigned_v<T>) {
                return Value(this, (u64)value);
            } else if constexpr (std::is_floating_point_v<T>) {
                return Value(this, (f64)value);
            } else {
                return Value(this, (i64)value);
            }
        }
    };
};