#include <source_map.h>
#include <parse.h>
using namespace std;

namespace gjs {
	void source_map::append(ast_node* node) {
		elem e;
		bool found_file = false;
		bool found_line = false;
		for (u16 i = 0;i < files.size();i++) {
			if (files[i] == node->start.file) {
				e.file = i;
				found_file = true;
			}
		}
		for (u32 i = 0;i < lines.size();i++) {
			if (lines[i] == node->start.lineText) {
				e.line = i;
				found_line = true;
			}
		}
		if (!found_file) {
			e.file = files.size();
			files.push_back(node->start.file);
		}
		if (!found_line) {
			e.line = lines.size();
			lines.push_back(node->start.lineText);
		}

		e.col = node->start.col;

		map.push_back(e);
	}

	source_map::src_info source_map::get(address addr) {
		elem e = map[addr];
		return { files[e.file], lines[e.line], e.line, e.col };
	}
};