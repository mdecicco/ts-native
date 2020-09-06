#pragma once
#include <string>
#include <vector>
#include <types.h>

namespace gjs {
	class vm_context;
	class source_map;
	class compile_log;
	class instruction_array;
	struct compile_context;
	struct func;
	struct data_type;

	namespace parse {
		struct ast;
	};

	class compile_exception : public std::exception {
		public:
			compile_exception(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col);
			compile_exception(const std::string& text, parse::ast* node);
			~compile_exception();

			virtual const char* what() { return text.c_str(); }
			std::string file;
			std::string text;
			std::string lineText;
			u32 line;
			u32 col;
	};

	void compile_node(compile_context& ctx, parse::ast* node);
	u16 count_references(parse::ast* node, const std::string& identifier, bool ignoreNext = false);
	void compile_ast(vm_context* ctx, parse::ast* tree, instruction_array* out, source_map* map, compile_log* log);
};
