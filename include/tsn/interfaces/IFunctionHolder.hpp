#pragma once
#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/utils/function_match.hpp>

namespace tsn {
    template <typename Ret, typename...Args>
    utils::Array<ffi::Function*> IFunctionHolder::findFunctions(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        function_match_flags flags
    ) {
        return function_match<Ret, Args...>(types, name, m_funcs, flags);
    }
};