#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/utils/function_match.h>
#include <tsn/ffi/Function.h>

namespace tsn {
    namespace ffi {
        FunctionRegistry::FunctionRegistry(Context* ctx) : IContextual(ctx) { }
        FunctionRegistry::~FunctionRegistry() { }

        void FunctionRegistry::registerFunction(ffi::Function* fn) {
            if (fn->m_registryIndex != u32(-1)) {
                throw "Function already registered";
            }

            fn->m_registryIndex = allFunctions().size();
            addFunction(fn);
        }

        utils::Array<ffi::Function*> FunctionRegistry::findFunctions(
            const utils::String& name,
            const ffi::DataType* retTp,
            const ffi::DataType** argTps,
            u8 argCount,
            function_match_flags flags
        ) {
            return IFunctionHolder::findFunctions(name, retTp, argTps, argCount, flags);
        }
    };
};