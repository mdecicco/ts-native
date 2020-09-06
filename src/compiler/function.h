#pragma once
#include <vector>
#include <string>
#include <register.h>

namespace gjs {
	class vm_function;
	struct compile_context;
	struct var;
	struct data_type;

	namespace parse {
		struct ast;
	};

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
		parse::ast* root;
		bool auto_free_consumed_vars;
		bool reached_return;
		bool is_thiscall;
		bool is_subtype_obj_ctor;
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

		var* allocate(compile_context& ctx, parse::ast* decl, bool is_arg = false);
		var* allocate(compile_context& ctx, data_type* type, bool is_arg = false);
		var* allocate(compile_context& ctx, const std::string& name, data_type* type, bool is_arg = false);
		var* allocate_stack_var(compile_context& ctx, data_type* type, parse::ast* because);
		var* imm(compile_context& ctx, integer i);
		var* imm(compile_context& ctx, f32 f);
		var* imm(compile_context& ctx, f64 d);
		var* imm(compile_context& ctx, char* s);
		var* zero(compile_context& ctx, data_type* type);
		void free(var* v);
		void free_stack_object(var* obj, parse::ast* because);
		void free_consumed_vars();
		var* get(compile_context& ctx, parse::ast* identifier);

		void push_scope();
		void pop_scope(compile_context& ctx, parse::ast* because);

		void generate_return_code(compile_context& ctx, parse::ast* because, address returns_stack_addr = UINT32_MAX);
		bool in_use(vmr reg);
	};

	void compile_function(compile_context& ctx, parse::ast* node, func* out);
	var* compile_variable_declaration(compile_context& ctx, parse::ast* node);
	var* call(compile_context& ctx, func* to, parse::ast* because, const std::vector<var*>& args, data_type* method_of = nullptr);
};
