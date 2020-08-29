#pragma once
#include <types.h>
#include <string>
#include <register.h>

namespace gjs {
	struct compile_context;
	struct data_type;
	struct ast_node;

	struct var {
		using vmr = vm_register;

		var();

		compile_context* ctx;
		std::string name;
		bool is_variable;
		bool no_auto_free;
		bool is_reg;
		bool is_imm;
		bool is_const;
		bool refers_to_stack_obj;
		uinteger refers_to_stack_addr;
		data_type* type;
		u32 count;
		u16 ref_count;
		u16 total_ref_count;

		union {
			vmr reg;
			address stack_addr;
		} loc;

		union {
			integer i;
			f32 f_32;
			f64 f_64;
		} imm;

		void move_stack_reference(var* to);

		address move_to_stack(ast_node* because, integer offset = 0);

		address move_to_stack(ast_node* because, vmr offset);

		vmr move_to_register(ast_node* because, integer offset = 0);

		vmr move_to_register(ast_node* because, vmr offset);

		vmr to_reg(ast_node* because, integer offset = 0);

		vmr to_reg(ast_node* because, vmr offset);

		address to_stack(ast_node* because, integer offset = 0);

		address to_stack(ast_node* because, vmr offset);

		void move_to(vmr reg, ast_node* because, integer stack_offset = 0);

		void move_to(vmr reg, ast_node* because, vmr stack_offset);

		void store_in(vmr reg, ast_node* because, integer stack_offset = 0);

		void store_in(vmr reg, ast_node* because, vmr stack_offset);
	};

	var* cast(compile_context& ctx, var* from, data_type* to, ast_node* because);

	var* cast(compile_context& ctx, var* from, var* to, ast_node* because);
};