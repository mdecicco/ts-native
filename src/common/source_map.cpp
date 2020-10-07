#include <common/source_map.h>
using namespace std;

namespace gjs {
    void source_map::append(source_ref src) {
        append(src.filename, src.line_text, src.line, src.col);
    }

    void source_map::append(const std::string& file, const std::string& lineText, u32 line, u32 col) {
        elem e;
        bool found_file = false;
        bool found_line = false;
        for (u16 i = 0;i < files.size();i++) {
            if (files[i] == file) {
                e.file = i;
                found_file = true;
            }
        }
        for (u32 i = 0;i < lines.size();i++) {
            if (lines[i] == lineText) {
                e.lineTextIdx = i;
                found_line = true;
            }
        }
        if (!found_file) {
            e.file = (u16)files.size();
            files.push_back(file);
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
        return source_ref(files[e.file], lines[e.lineTextIdx], e.line, e.col);
    }
};