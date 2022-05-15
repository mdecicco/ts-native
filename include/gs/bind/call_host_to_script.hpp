#pragma once
#include <gs/bind/call_common.hpp>
#include <gs/bind/ffi_argument.hpp>
#include <gs/common/Function.h>

namespace gs {
    namespace ffi {
        template <typename ...Args>
        void call_hostToScript(Function* f, void* result, Args&&... args) {
        }
    };
};
