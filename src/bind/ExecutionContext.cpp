#include <tsn/bind/ExecutionContext.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        ExecutionContext::ExecutionContext(Context* ctx) : IContextual(ctx) {

        }

        ExecutionContext::~ExecutionContext() {

        }
    };
};