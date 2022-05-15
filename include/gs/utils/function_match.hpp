#pragma once
#include <gs/bind/bind.h>
#include <gs/utils/function_match.h>
#include <gs/utils/remove_all.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>

namespace gs {
    template <typename T>
    struct _remove_all_except_void_ptr { using type = ffi::remove_all<T>::type; };

    template <>
    struct _remove_all_except_void_ptr<void*> { using type = void*; };

    template <typename Ret, typename...Args>
    utils::Array<ffi::Function*> function_match(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    ) {
        constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

        ffi::DataType* retTp = types->getType<_remove_all_except_void_ptr<Ret>::type>();
        if (!retTp) {
            throw ffi::BindException(utils::String::Format(
                "function_match: Provided return type '%s' has not been bound",
                type_name<Ret>()
            ));
        }

        ffi::DataType* argTps[] = { types->getType<_remove_all_except_void_ptr<Args>::type>()... };
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