#pragma once
#include <vector>
#include <string>
#include <gjs/common/types.h>

namespace gjs {
    class script_context;
    class script_function;
    class script_type;
    class vm_backend;
    struct compilation_output;

    std::vector<std::string> split(const std::string& str, const std::string& delimiters);

    std::vector<std::string> get_lines(const std::string& str);

    std::string format(const char* fmt, ...);

    u32 hash(const std::string& str);

    inline u64 join_u32(u32 a, u32 b) { return (u64(a) << 32) | b; }

    inline u32 extract_left_u32(u64 joined) { return ((u32*)&joined)[1]; }

    inline u32 extract_right_u32(u64 joined) { return ((u32*)&joined)[0]; }

    script_function* function_search(const std::string& name, const std::vector<script_function*>& source, script_type* ret, const std::vector<script_type*>& arg_types);

    void print_log(script_context* ctx, u32 context_line_count = 5);

    void print_code(vm_backend* ctx);

    void debug_ir_step(script_context* ctx, compilation_output& in);
};

