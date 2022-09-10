#include <gs/common/FunctionRegistry.h>
#include <gs/utils/function_match.h>
#include <gs/common/Function.h>

namespace gs {
    namespace ffi {
        FunctionRegistry::FunctionRegistry(Context* ctx) : IContextual(ctx) { }
        FunctionRegistry::~FunctionRegistry() { }

        void FunctionRegistry::registerFunction(ffi::Function* fn) {
            if (fn->m_id != -1) {
                throw std::exception("Function already registered");
            }

            fn->m_id = allFunctions().size();
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