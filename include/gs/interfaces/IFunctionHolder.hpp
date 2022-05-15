#pragma once
#include <gs/interfaces/IFunctionHolder.h>
#include <gs/utils/function_match.hpp>

namespace gs {
    template <typename Ret, typename...Args>
    utils::Array<ffi::Function*> IFunctionHolder::findFunctions(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    ) {
        return function_match<Ret, Args...>(types, name, funcs, flags);
    }
};