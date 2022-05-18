#include <gs/bind/ExecutionContext.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        ExecutionContext::ExecutionContext(Context* ctx) : IContextual(ctx) {

        }

        ExecutionContext::~ExecutionContext() {

        }
    };
};