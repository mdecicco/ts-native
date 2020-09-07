#pragma once
#include <compiler/variable.h>
#include <compiler/tac.h>

namespace gjs {
	class vm_context;
	class type_manager;
	class compile_log;

	namespace parse {
		struct ast;
	};

	namespace compile {
		struct context {
			vm_context* env;
			parse::ast* input;
			type_manager* new_types;
			std::vector<vm_function*> new_functions;
			std::vector<tac_instruction> code;
			u32 next_reg_id;
			parse::ast* rel_node;

			context();
			var imm(u64 u);
			var imm(i64 i);
			var imm(f32 f);
			var imm(f64 d);
			var imm(const std::string& s);
			var new_var(vm_type* type, const std::string& name);
			var new_var(vm_type* type);
			var dyn_var(vm_type* type, const std::string& name);
			var dyn_var(vm_type* type);
			var clone_var(var v);
			var empty_var(vm_type* type);
			var dummy_var(vm_type* type, const std::string& name);
			var dummy_var(vm_type* type);

			vm_function* function(const std::string& name, vm_type* ret, const std::vector<vm_type*>& args);
			vm_type* type(const std::string& name);
			tac_instruction& add(operation op);
			void push_node(parse::ast* node);
			void pop_node();
			parse::ast* node();
			compile_log* log();
		};
	};
};