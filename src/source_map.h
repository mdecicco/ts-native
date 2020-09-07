#pragma once
#include <vector>
#include <string>
#include <types.h>
#include <source_ref.h>

namespace gjs {
    struct source_map {
        std::vector<std::string> files;
        std::vector<std::string> lines;
        struct elem {
            u16 file;
            u32 lineTextIdx;
            u32 line;
            u32 col;
        };

        // 1:1 map, instruction address : elem
        std::vector<elem> map;

        void append(source_ref src);
        void append(const std::string& file, const std::string& lineText, u32 line, u32 col);

        source_ref get(address addr);
    };
};