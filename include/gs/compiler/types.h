#pragma once
#include <gs/common/types.h>

#include <xtr1common>

namespace gs {
    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        typedef u32 vreg_id;
        typedef u32 label_id;
        typedef u32 alloc_id;

        template <typename T>
        struct is_imm {
            static constexpr bool value = std::is_integral_v<T> || std::is_same_v<T, ffi::Function*>;
        };

        template <typename T>
        inline constexpr bool is_imm_v = is_imm<T>::value;


        constexpr u32 invalid_reg = 0;
        constexpr u32 invalid_label = 0;
    };
};