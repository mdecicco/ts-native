#include <gs/common/FunctionRegistry.h>
#include <gs/utils/function_match.h>

namespace gs {
    namespace ffi {
        FunctionRegistry::FunctionRegistry(Context* ctx) : IContextual(ctx) { }
        FunctionRegistry::~FunctionRegistry() { }

        utils::Array<ffi::Function*> FunctionRegistry::findFunctions(
            const utils::String& name,
            ffi::DataType* retTp,
            ffi::DataType** argTps,
            u8 argCount,
            const utils::Array<ffi::Function*>& funcs,
            function_match_flags flags
        ) {
            return function_match(name, retTp, argTps, argCount, funcs, flags);
        }
    };
};