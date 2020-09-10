#pragma once
#include <vector>
#include <string>
#include <common/types.h>

namespace gjs {
    std::vector<std::string> split(const std::string& str, const std::string& delimiters);
    std::vector<std::string> get_lines(const std::string& str, const std::string& delimiters);
    std::string format(const char* fmt, ...);
    u32 hash(const std::string& str);
};

