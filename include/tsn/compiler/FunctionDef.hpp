#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/Logger.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Function.h>
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
            } else {
                ffi::DataType* tp = m_comp->getContext()->getTypes()->getType<T>();

                if constexpr (std::is_unsigned_v<T>) {
                    Value ret = Value(this, (u64)value);
                    ret.setType(tp);
                    return ret;
                } else if constexpr (std::is_floating_point_v<T>) {
                    Value ret = Value(this, (f64)value);
                    ret.setType(tp);
                    return ret;
                }
                
                Value ret = Value(this, (i64)value);
                ret.setType(tp);
                return ret;
            }
        }
    };
};