#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IFunctionHolder.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace gs {
    namespace ffi {
        class FunctionRegistry : public IContextual, public IFunctionHolder {
            public:
                FunctionRegistry(Context* ctx);
                ~FunctionRegistry();
        };
    };
};