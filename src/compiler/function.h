#pragma once
#include <vector>
#include <string>
#include <register.h>

namespace gjs {
	class vm_function;
	struct ast_node;
	struct compile_context;
	struct func;
	struct var;
	struct data_type;

	struct func {
		using vmr = vm_register;
		struct stack_slot {
			address addr;
			u32 size;
			bool used;
		};

		struct arg_info {
			std::string name;
			data_type* type;
			vmr loc;
		};

		struct scope {
			std::vector<var*> vars;
			std::vector<std::pair<address, data_type*>> stack_objects;
		};

		std::string name;
		data_type* this_type;
		data_type* return_type;
		bool return_loc_determined;
		bool returns_on_stack;
		vmr return_loc;
		bool is_ctor;
		bool is_dtor;
		bool returns_pointer;
		std::vector<arg_info> args;
		u64 entry;
		std::vector<scope> scopes;
		ast_node* root;
		bool auto_free_consumed_vars;
		bool reached_return;
		bool is_thiscall;
		vm_function* bound_to;

		struct _stack {
			func* parent;
			std::vector<stack_slot> slots;

			address allocate(u32 size);

			void free(address addr);

			u32 size();
		} stack;

		struct _registers {
			func* parent;
			std::vector<vmr> a;
			std::vector<vmr> gp;
			std::vector<vmr> fp;
			std::vector<vmr> used;

			vmr allocate_arg();

			vmr allocate_gp();

			vmr allocate_fp();

			void free(vmr reg);
		} registers;

		func();
		func(compile_context& ctx, vm_function* f);

		var* allocate(compile_context& ctx, ast_node* decl, bool is_arg = false);
		var* allocate(compile_context& ctx, data_type* type, bool is_arg = false);
		var* allocate(compile_context& ctx, const std::string& name, data_type* type, bool is_arg = false);
		var* allocate_stack_var(compile_context& ctx, data_type* type, ast_node* because);
		var* imm(compile_context& ctx, integer i);
		var* imm(compile_context& ctx, f32 f);
		var* imm(compile_context& ctx, f64 d);
		var* imm(compile_context& ctx, char* s);
		var* zero(compile_context& ctx, data_type* type);
		void free(var* v);
		void free_stack_object(var* obj, ast_node* because);
		void free_consumed_vars();
		var* get(compile_context& ctx, ast_node* identifier);

		void push_scope();
		void pop_scope(compile_context& ctx, ast_node* because);

		void generate_return_code(compile_context& ctx, ast_node* because, address returns_stack_addr = UINT32_MAX);
		bool in_use(vmr reg);
	};

	void compile_function(compile_context& ctx, ast_node* node, func* out);
	void compile_variable_declaration(compile_context& ctx, ast_node* node);
	var* call(compile_context& ctx, func* to, ast_node* because, const std::vector<var*>& args);
};
