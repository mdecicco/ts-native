#pragma once
#include <gs/bind/bind.h>
#include <gs/utils/function_match.h>
#include <gs/utils/remove_all.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>

namespace gs {
    template <typename Ret, typename...Args>
    utils::Array<ffi::Function*> function_match(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    ) {
        constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

        ffi::DataType* retTp = types->getType<Ret>();
        if (!retTp) {
            throw ffi::BindException(utils::String::Format(
                "function_match: Provided return type '%s' has not been bound",
                type_name<Ret>()
            ));
        }

        ffi::DataType* argTps[] = { types->getType<Args>()... };
        for (u8 a = 0;a < argc;a++) {
            if (!argTps[a]) {
                throw ffi::BindException(utils::String::Format(
                    "function_match: Provided argument type '%s' at index '%d' has not been bound",
                    type_name<Ret>(), a
                ));
            }
        }

        return function_match(name, retTp, argTps, argc, funcs, flags | fm_strict);
    }

    template <typename Ret, typename...Args>
    utils::Array<ffi::Method*> function_match(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Method*>& funcs,
        function_match_flags flags
    ) {
        constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

        ffi::DataType* retTp = types->getType<Ret>();
        if (!retTp) {
            throw ffi::BindException(utils::String::Format(
                "function_match: Provided return type '%s' has not been bound",
                type_name<Ret>()
            ));
        }

        ffi::DataType* argTps[] = { types->getType<Args>()... };
        for (u8 a = 0;a < argc;a++) {
            if (!argTps[a]) {
                throw ffi::BindException(utils::String::Format(
                    "function_match: Provided argument type '%s' at index '%d' has not been bound",
                    type_name<Ret>(), a
                ));
            }
        }

        return function_match(name, retTp, argTps, argc, funcs, flags | fm_strict);
    }
};