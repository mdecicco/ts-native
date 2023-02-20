#pragma once
#include <tsn/bind/call_common.hpp>
#include <tsn/bind/ffi_argument.hpp>
#include <tsn/ffi/Function.h>

namespace tsn {
    namespace ffi {
        template <typename ...Args>
        void call_hostToScript(Context* ctx, Function* f, void* result, Args&&... args) {
        }
    };
};
