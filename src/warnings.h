#pragma once
#include <types.h>
#include <source_ref.h>
#include <exception>

namespace gjs {
    namespace warning {
        enum class wcode {
            no_warning = 0,
            __parse_warning_start,
            __parse_warning_end,
            __compile_warning_start,
            __compile_warning_end
        };

        const char* format_str(wcode code);
    };
};