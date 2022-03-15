#include <gjs/common/source_ref.h>

namespace gjs {
    source_ref::source_ref() : line(0), col(0) { }
    source_ref::source_ref(const source_ref& r) {
        module = r.module;
        line_text = r.line_text;
        line = r.line;
        col = r.col;

        auto p = line_text.find_first_of('\n');
        while (p != std::string::npos) {
            line_text.erase(p);
            p = line_text.find_first_of('\n');
            if (p == std::string::npos) p = line_text.find_first_of('\r');
        }
    }
    source_ref::source_ref(const std::string& _module, const std::string& line_txt, u16 _line, u16 _col) {
        module = _module;
        line_text = line_txt;
        line = _line;
        col = _col;

        auto p = line_text.find_first_of('\n');
        while (p != std::string::npos) {
            line_text.erase(p);
            p = line_text.find_first_of('\n');
            if (p == std::string::npos) p = line_text.find_first_of('\r');
        }
    }
    void source_ref::set(const std::string& _module, const std::string& line_txt, u16 _line, u16 _col) {
        module = _module;
        line_text = line_txt;
        line = _line;
        col = _col;

        auto p = line_text.find_first_of('\n');
        while (p != std::string::npos) {
            line_text.erase(p);
            p = line_text.find_first_of('\n');
            if (p == std::string::npos) p = line_text.find_first_of('\r');
        }
    }
};