#pragma once
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/utils/function_match.hpp>

namespace tsn {
    namespace ffi {
        template <typename Ret, typename...Args>
        utils::Array<ffi::Function*> FunctionRegistry::findFunctions(
            const utils::String& name,
            function_match_flags flags
        ) {
            return IFunctionHolder::findFunctions<Ret, Args...>(m_ctx->getTypes(), name, flags);
        }
    };
};