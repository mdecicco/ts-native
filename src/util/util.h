#pragma once
#include <vector>
#include <string>
#include <common/types.h>

namespace gjs {
    std::vector<std::string> split(const std::string& str, const std::string& delimiters);
    std::vector<std::string> get_lines(const std::string& str, const std::string& delimiters);
    std::string format(const char* fmt, ...);
    u32 hash(const std::string& str);
    inline u64 join_u32(u32 a, u32 b) { return (u64(a) << 32) | b; }
    inline u32 extract_left_u32(u64 joined) { return ((u32*)&joined)[1]; }
    inline u32 extract_right_u32(u64 joined) { return ((u32*)&joined)[0]; }
};

