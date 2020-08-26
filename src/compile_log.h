#pragma once
#include <string>
#include <vector>
#include <types.h>

namespace gjs {
	struct ast_node;

	struct compile_message {
		std::string file;
		std::string text;
		std::string lineText;
		u32 line;
		u32 col;
	};

	class compile_log {
		public:
			compile_log() { }
			~compile_log() { }

			void err(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col);
			void err(const std::string& text, ast_node* node);

			void warn(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col);
			void warn(const std::string& text, ast_node* node);

			std::vector<compile_message> errors;
			std::vector<compile_message> warnings;
	};
};

