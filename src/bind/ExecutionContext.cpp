#include <gs/bind/ExecutionContext.h>

namespace gs {
    namespace ffi {
        ExecutionContext::ExecutionContext(Context* ctx) : IContextual(ctx) {

        }

        ExecutionContext::~ExecutionContext() {

        }
    };
};