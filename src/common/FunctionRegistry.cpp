#include <gs/common/FunctionRegistry.h>
#include <gs/utils/function_match.h>
#include <gs/common/Function.h>

namespace gs {
    namespace ffi {
        FunctionRegistry::FunctionRegistry(Context* ctx) : IContextual(ctx) { }
        FunctionRegistry::~FunctionRegistry() { }

        utils::Array<ffi::Function*> FunctionRegistry::findFunctions(
            const utils::String& name,
            ffi::DataType* retTp,
            ffi::DataType** argTps,
            u8 argCount,
            function_match_flags flags
        ) {
            return IFunctionHolder::findFunctions(name, retTp, argTps, argCount, flags);
        }
    };
};