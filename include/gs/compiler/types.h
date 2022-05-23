#pragma once
#include <gs/common/types.h>

namespace gs {
    namespace compiler {
        typedef u32 vreg_id;
        typedef u32 label_id;
        typedef u32 alloc_id;

        constexpr u32 invalid_reg = 0;
        constexpr u32 invalid_label = 0;
    };
};