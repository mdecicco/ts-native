#pragma once
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>

namespace gjs {
    // Get script_type that represents some type T
    template <typename T>
    script_type* type_of(script_context* ctx, bool do_throw = false);
};

#include <gjs/util/typeof.inl>