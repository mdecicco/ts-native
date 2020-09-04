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
	struct var;

	struct compile_context {
		vm_context* ctx;
		compile_log* log;
		instruction_array* out;
		func* cur_func;
		source_map* map;
		std::vector<data_type*> types;
		std::vector<func*> funcs;

		// used when compiling expressions involving type properties
		bool do_store_member_info;
		struct {
			bool is_set;
			std::string name;
			data_type* type;
			var* subject;
			func* method;
		} last_member_or_method;

		void clear_last_member_info();

		void add(instruction& i, ast_node* because);

		data_type* type(const std::string& name);

		data_type* type(ast_node* node);

		func* function(const std::string& name);
	};
};