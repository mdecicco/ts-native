#include <compiler/variable.h>
#include <compiler/context.h>
#include <compiler/data_type.h>
#include <compiler/function.h>
#include <compiler/compiler.h>
#include <compile_log.h>
#include <util.h>
#include <instruction_encoder.h>
#include <instruction.h>
#include <register.h>

#define is_fp(r) (r >= vmr::f0 && r <= vmr::f15)
#define is_arg(r) (r >= vmr::a0 && r <= vmr::a7)

namespace gjs {
	using vmr = vm_register;
	using vmi = vm_instruction;
	
	var::var() {
		ctx = nullptr;
		is_variable = false;
		no_auto_free = false;
		is_reg = false;
		is_imm = false;
		is_const = false;
		refers_to_stack_obj = false;
		type = nullptr;
		count = 1;
		total_ref_count = 0;
		ref_count = 0;
	}

	void var::move_stack_reference(var* to) {
		to->refers_to_stack_obj = refers_to_stack_obj;
		to->refers_to_stack_addr = refers_to_stack_addr;
	}

	address var::move_to_stack(ast_node* because, integer offset) {
		if (!is_reg && !is_imm) return loc.stack_addr;
		vmi store;
		switch (type->size) {
			case 1: { store = vmi::st8; break; }
			case 2: { store = vmi::st16; break; }
			case 4: { store = vmi::st32; break; }
			case 8: { store = vmi::st64; break; }
		}
		address addr = ctx->cur_func->stack.allocate(type->size);
		ctx->add(
			encode(store).operand(loc.reg).operand(vmr::sp).operand(uinteger(addr) + offset),
			because
		);

		ctx->cur_func->registers.free(loc.reg);
		is_reg = false;
		loc.stack_addr = addr;

		return addr;
	}

	address var::move_to_stack(ast_node* because, vmr offset) {
		if (!is_reg && !is_imm) return loc.stack_addr;
		vmi store;
		switch (type->size) {
		case 1: { store = vmi::st8; break; }
		case 2: { store = vmi::st16; break; }
		case 4: { store = vmi::st32; break; }
		case 8: { store = vmi::st64; break; }
		}
		address addr = ctx->cur_func->stack.allocate(type->size);
		ctx->add(
			encode(vmi::addui).operand(vmr::v0).operand(offset).operand(addr),
			because
		);

		ctx->add(
			encode(store).operand(loc.reg).operand(vmr::v0),
			because
		);

		ctx->cur_func->registers.free(loc.reg);
		is_reg = false;
		loc.stack_addr = addr;

		return addr;
	}

	vmr var::move_to_register(ast_node* because, integer offset) {
		vmr reg;
		if (type->is_floating_point) reg = ctx->cur_func->registers.allocate_fp();
		else reg = ctx->cur_func->registers.allocate_gp();
		ctx->cur_func->registers.free(reg);

		move_to(reg, because, offset);

		return reg;
	}

	vmr var::move_to_register(ast_node* because, vmr offset) {
		vmr reg;
		if (type->is_floating_point) reg = ctx->cur_func->registers.allocate_fp();
		else reg = ctx->cur_func->registers.allocate_gp();
		ctx->cur_func->registers.free(reg);

		move_to(reg, because, offset);

		return reg;
	}

	vmr var::to_reg(ast_node* because, integer offset) {
		if (is_reg) return loc.reg;
		return move_to_register(because, offset);
	}

	vmr var::to_reg(ast_node* because, vmr offset) {
		if (is_reg) return loc.reg;
		return move_to_register(because, offset);
	}

	address var::to_stack(ast_node* because, integer offset) {
		if (!is_reg) return loc.stack_addr;
		return move_to_stack(because, offset);
	}

	address var::to_stack(ast_node* because, vmr offset) {
		if (!is_reg) return loc.stack_addr;
		return move_to_stack(because, offset);
	}

	void var::move_to(vmr reg, ast_node* because, integer stack_offset) {
		bool found = false;
		if (is_fp(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.fp.size();i++) {
				if (ctx->cur_func->registers.fp[i] == reg) {
					found = true;
					ctx->cur_func->registers.fp.erase(ctx->cur_func->registers.fp.begin() + i);
					break;
				}
			}
		} else if (is_arg(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.a.size();i++) {
				if (ctx->cur_func->registers.a[i] == reg) {
					found = true;
					ctx->cur_func->registers.a.erase(ctx->cur_func->registers.a.begin() + i);
					break;
				}
			}
		} else {
			for (u32 i = 0;i < ctx->cur_func->registers.gp.size();i++) {
				if (ctx->cur_func->registers.gp[i] == reg) {
					found = true;
					ctx->cur_func->registers.gp.erase(ctx->cur_func->registers.gp.begin() + i);
					break;
				}
			}
		}

		if (!found) {
			throw compile_exception("Register not available for var to be moved to", because);
		}

		ctx->cur_func->registers.used.push_back(reg);
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
			ctx->cur_func->registers.free(loc.reg);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
			case 1: { load = vmi::ld8; break; }
			case 2: { load = vmi::ld16; break; }
			case 4: { load = vmi::ld32; break; }
			case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(load).operand(reg).operand(vmr::sp).operand(loc.stack_addr + stack_offset),
				because
			);
			ctx->cur_func->stack.free(loc.stack_addr);
		}

		loc.reg = reg;
		is_reg = true;
		is_imm = false;
	}

	void var::move_to(vmr reg, ast_node* because, vmr stack_offset) {
		bool found = false;
		if (is_fp(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.fp.size();i++) {
				if (ctx->cur_func->registers.fp[i] == reg) {
					found = true;
					ctx->cur_func->registers.fp.erase(ctx->cur_func->registers.fp.begin() + i);
					break;
				}
			}
		} else if (is_arg(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.a.size();i++) {
				if (ctx->cur_func->registers.a[i] == reg) {
					found = true;
					ctx->cur_func->registers.a.erase(ctx->cur_func->registers.a.begin() + i);
					break;
				}
			}
		} else {
			for (u32 i = 0;i < ctx->cur_func->registers.gp.size();i++) {
				if (ctx->cur_func->registers.gp[i] == reg) {
					found = true;
					ctx->cur_func->registers.gp.erase(ctx->cur_func->registers.gp.begin() + i);
					break;
				}
			}
		}

		if (!found) {
			throw compile_exception("Register not available for var to be moved to", because);
		}

		ctx->cur_func->registers.used.push_back(reg);
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
			ctx->cur_func->registers.free(loc.reg);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
			case 1: { load = vmi::ld8; break; }
			case 2: { load = vmi::ld16; break; }
			case 4: { load = vmi::ld32; break; }
			case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(vmi::addu).operand(vmr::v0).operand(vmr::sp).operand(stack_offset),
				because
			);

			ctx->add(
				encode(load).operand(reg).operand(vmr::v0).operand(loc.stack_addr),
				because
			);
			ctx->cur_func->stack.free(loc.stack_addr);
		}

		loc.reg = reg;
		is_reg = true;
		is_imm = false;
	}

	void var::store_in(vmr reg, ast_node* because, integer stack_offset) {
		if (is_reg) {
			if (loc.reg == reg) return;
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
			case 1: { load = vmi::ld8; break; }
			case 2: { load = vmi::ld16; break; }
			case 4: { load = vmi::ld32; break; }
			case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(load).operand(reg).operand(vmr::sp).operand(loc.stack_addr + stack_offset),
				because
			);
		}
	}

	void var::store_in(vmr reg, ast_node* because, vmr stack_offset) {
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
			case 1: { load = vmi::ld8; break; }
			case 2: { load = vmi::ld16; break; }
			case 4: { load = vmi::ld32; break; }
			case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(vmi::add).operand(vmr::v0).operand(vmr::sp).operand(stack_offset),
				because
			);

			ctx->add(
				encode(load).operand(reg).operand(vmr::v0).operand(loc.stack_addr),
				because
			);
		}
	}



	var* cast(compile_context& ctx, var* from, data_type* to, ast_node* because) {
		if (from->type->equals(to)) return from;
		if (from->type->name == "bool" || to->name == "bool") return from;

		if (!from->type->built_in || (from->type->name == "string" || to->name == "string")) {
			func* cast_func = from->type->cast_to_func(to);
			if (!cast_func) {
				ctx.log->err(format("No conversion from '%s' to '%s' found", from->type->name.c_str(), to->name.c_str()), because);
				return from;
			}

			return call(ctx, cast_func, because, { from });
		}

		if (!to->built_in) {
			ctx.log->err(format("Cannot convert from '%s' to '%s'", from->type->name.c_str(), to->name.c_str()), because);
			return from;
		}

		if (from->type->name == "void") {
			ctx.log->err(format("Cannot convert from 'void' to '%s'", to->name.c_str()), because);
			return from;
		}
		if (to->name == "void") {
			ctx.log->err(format("Cannot convert from '%s' to 'void'", to->name.c_str()), because);
			return from;
		}

		// only types remaining are integral
		// todo: conversion between f32/f64
		if (!from->type->is_floating_point) {
			if (from->is_imm) return ctx.cur_func->imm(ctx, (decimal)from->imm.i);
			var* tmp = ctx.cur_func->allocate(ctx, to);
			ctx.add(
				encode(vmi::mtfp).operand(from->to_reg(because)).operand(tmp->to_reg(because)),
				because
			);
			ctx.add(
				encode(vmi::ctf).operand(tmp->to_reg(because)),
				because
			);
			return tmp;
		} else {
			if (from->is_imm) return ctx.cur_func->imm(ctx, (integer)from->imm.d);
			var* tmp = ctx.cur_func->allocate(ctx, to);
			vmr treg = ctx.cur_func->registers.allocate_fp();
			ctx.add(
				encode(vmi::fadd).operand(treg).operand(from->to_reg(because)).operand(vmr::zero),
				because
			);
			ctx.add(
				encode(vmi::cti).operand(treg),
				because
			);
			ctx.add(
				encode(vmi::mffp).operand(treg).operand(tmp->to_reg(because)),
				because
			);
			ctx.cur_func->registers.free(treg);
			return tmp;
		}

		ctx.log->err(format("No conversion from '%s' to '%s' found", from->type->name.c_str(), to->name.c_str()), because);
		return from;
	}

	var* cast(compile_context& ctx, var* from, var* to, ast_node* because) {
		if (!to) return from;
		return cast(ctx, from, to->type, because);
	}
};