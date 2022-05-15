#pragma once
#include <gs/common/FunctionRegistry.h>
#include <gs/common/Context.h>
#include <gs/utils/function_match.hpp>

namespace gs {
    namespace ffi {
        template <typename Ret, typename...Args>
        utils::Array<ffi::Function*> FunctionRegistry::findFunctions(
            const utils::String& name,
            const utils::Array<ffi::Function*>& funcs,
            function_match_flags flags
        ) {
            return function_match<Ret, Args...>(m_ctx->getTypes(), name, funcs, flags);
        }
    };
};