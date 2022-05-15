#include <gs/common/FunctionRegistry.h>

namespace gs {
    namespace ffi {
        FunctionRegistry::FunctionRegistry(Context* ctx) : IContextual(ctx) { }
        FunctionRegistry::~FunctionRegistry() { }
    };
};