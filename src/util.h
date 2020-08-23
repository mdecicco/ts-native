#pragma once
#include <vector>
#include <string>

namespace gjs {
	std::vector<std::string> split(const std::string& str, const std::string& delimiters);
	std::string format(const char* fmt, ...);
};

