#pragma once
#include <vector>
#include <string>
#include <types.h>

namespace gjs {
	struct ast_node;

	struct source_map {
		std::vector<std::string> files;
		std::vector<std::string> lines;
		struct elem {
			u16 file;
			u32 lineTextIdx;
			u32 line;
			u32 col;
		};
		struct src_info {
			std::string file;
			std::string lineText;
			u32 line;
			u32 col;
		};

		// 1:1 map, instruction address : elem
		std::vector<elem> map;

		void append(ast_node* node);
		void append(const std::string& file, const std::string& lineText, u32 line, u32 col);

		src_info get(address addr);
	};
};