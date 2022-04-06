#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <vector>
#include <string>

namespace gjs {
    class source_map {
        public:
            std::vector<std::string> modules;
            std::vector<std::string> lines;
            struct elem {
                u16 moduleIdx;
                u32 lineTextIdx;
                u32 line;
                u32 col;
            };

            // 1:1 map, instruction address : elem
            std::vector<elem> map;

            void append(source_ref src);
            void append(const std::string& module, const std::string& lineText, u32 line, u32 col);

            source_ref get(address addr);
    };
};