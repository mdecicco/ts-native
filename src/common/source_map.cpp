#include <common/source_map.h>
using namespace std;

namespace gjs {
    void source_map::append(source_ref src) {
        append(src.module, src.line_text, src.line, src.col);
    }

    void source_map::append(const std::string& module, const std::string& lineText, u32 line, u32 col) {
        elem e;
        bool found_module = false;
        bool found_line = false;
        for (u16 i = 0;i < modules.size();i++) {
            if (modules[i] == module) {
                e.moduleIdx = i;
                found_module = true;
            }
        }
        for (u32 i = 0;i < lines.size();i++) {
            if (lines[i] == lineText) {
                e.lineTextIdx = i;
                found_line = true;
            }
        }
        if (!found_module) {
            e.moduleIdx = (u16)modules.size();
            modules.push_back(module);
        }
        if (!found_line) {
            e.lineTextIdx = (u32)lines.size();
            lines.push_back(lineText);
        }

        e.line = line;
        e.col = col;

        map.push_back(e);
    }

    source_ref source_map::get(address addr) {
        elem e = map[addr];
        return source_ref(modules[e.moduleIdx], lines[e.lineTextIdx], e.line, e.col);
    }
};