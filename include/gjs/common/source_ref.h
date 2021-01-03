#pragma once
#include <gjs/common/types.h>
#include <string>

namespace gjs {
    struct source_ref {
        std::string module;
        std::string line_text;
        u16 line;
        u16 col;

        source_ref();
        source_ref(const source_ref& r);
        source_ref(const std::string& module, const std::string& lineTxt, u16 line, u16 col);
        void set(const std::string& module, const std::string& lineTxt, u16 line, u16 col);
    };
};
