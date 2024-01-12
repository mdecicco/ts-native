#pragma once
#include <tsn/common/types.h>

#ifdef _MSC_VER
#include <xtr1common>
#else
#include <type_traits>
#endif

namespace utils {
    template <typename T>
    class Array;
}

namespace tsn {
    class Module;

    namespace ffi {
        class DataType;
    };

    namespace compiler {
        class FunctionDef;
        class Compiler;
        class Value;

        typedef u32 vreg_id;
        typedef u32 label_id;
        typedef u32 alloc_id;

        template <typename T>
        struct is_imm {
            static constexpr bool value = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, FunctionDef*> || std::is_same_v<T, Module*>;
        };

        template <typename T>
        inline constexpr bool is_imm_v = is_imm<T>::value;


        constexpr u32 invalid_reg = 0;
        constexpr u32 invalid_label = 0;

        struct InlineCodeGenContext {
            Compiler* compiler;
            const Value* resultStorage;
            const Value* selfPointer;
            const utils::Array<Value>& arguments;
        };

        typedef void (*InlineCodeGenFunc)(compiler::InlineCodeGenContext* ctx);
    };
};