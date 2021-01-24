#pragma once
#include <gjs/compiler/variable.h>

namespace gjs {
    class script_function;

    namespace compile {
        struct context;

        std::string arg_tp_str(const std::vector<script_type*> types);

        var call(context& ctx, script_function* func, const std::vector<var>& args, const var* self = nullptr);
    };
};