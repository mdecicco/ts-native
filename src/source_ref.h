#pragma once
#include <types.h>
#include <string>

namespace gjs {
    struct source_ref {
        std::string filename;
        std::string line_text;
        u16 line;
        u16 col;

        source_ref();
        source_ref(const source_ref& r);
        source_ref(const std::string& file, const std::string& lineTxt, u16 line, u16 col);
        void set(const std::string& file, const std::string& lineTxt, u16 line, u16 col);
    };
};
