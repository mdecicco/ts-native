#pragma once
#include <string>
#include <vector>

namespace gjs {
	class instruction;
	class compile_log;
	class instruction_array;
	class source_map;
	class vm_context;
	struct func;
	struct ast_node;
	struct data_type;

	struct compile_context {
		vm_context* ctx;
		compile_log* log;
		instruction_array* out;
		func* cur_func;
		source_map* map;
		std::vector<data_type*> types;
		std::vector<func*> funcs;

		// used when compiling expressions involving type properties
		bool do_store_member_pointer;
		bool last_member_was_pointer;

		// used when compiling expressions involving type methods
		func* last_type_method;

		void add(instruction& i, ast_node* because);

		data_type* type(const std::string& name);

		data_type* type(ast_node* node);

		func* function(const std::string& name);
	};
};