#pragma once
#include <compiler/function.h>
#include <compiler/context.h>
#include <compiler/variable.h>
#include <compiler/data_type.h>
#include <compile_log.h>
#include <bind.h>
#include <instruction_array.h>
#include <register.h>
#include <parse.h>
#include <vm_function.h>
#include <vm_type.h>

#define is_fp(r) (r >= vmr::f0 && r <= vmr::f15)
#define is_arg(r) (r >= vmr::a0 && r <= vmr::a7)

using namespace std;

namespace gjs {
	using vmr = vm_register;
	using vmi = vm_instruction;
	static u32 anon_var_id = 0;

	func::func() {
		registers.a = {
			vmr::a0, vmr::a1, vmr::a2, vmr::a3, vmr::a4, vmr::a5, vmr::a6, vmr::a7
		};

		registers.gp = {
			vmr::s0, vmr::s1, vmr::s2, vmr::s3, vmr::s4, vmr::s5, vmr::s6, vmr::s7
		};

		registers.fp = {
			vmr::f0 , vmr::f1 , vmr::f2 , vmr::f3 , vmr::f4 , vmr::f5 , vmr::f6 , vmr::f7 ,
			vmr::f8 , vmr::f9 , vmr::f10, vmr::f11, vmr::f12, vmr::f13, vmr::f14, vmr::f15
		};

		returns_on_stack = false;
		return_loc = vmr::register_count;
		return_type = nullptr;
		root = nullptr;
		auto_free_consumed_vars = true;
		stack.parent = this;
		registers.parent = this;
		returns_pointer = false;
		is_ctor = false;
		is_dtor = false;
		this_type = nullptr;
		reached_return = false;
		return_loc_determined = false;
		is_subtype_obj_ctor = false;
		is_thiscall = false;
		bound_to = nullptr;
	}

	func::func(compile_context& ctx, vm_function* f) {
		name = f->name;
		return_type = ctx.type(f->signature.return_type->name);
		for (u8 a = 0;a < f->signature.arg_types.size();a++) {
			vmr loc = f->is_host ? f->signature.arg_locs[a] : vmr::register_count;
			args.push_back({ "", ctx.type(f->signature.arg_types[a]->name), loc });
		}
		entry = f->is_host ? f->access.wrapped->address : f->access.entry;
		returns_on_stack = f->signature.returns_on_stack;
		return_loc = f->signature.return_loc;
		returns_pointer = f->signature.returns_pointer;
		is_ctor = is_dtor = reached_return = false;
		return_loc_determined = true;
		is_thiscall = f->signature.is_thiscall;
		is_subtype_obj_ctor = f->signature.is_subtype_obj_ctor;
		bound_to = f;
	}

	void func::free(var* v) {
		if (v->is_reg) registers.free(v->loc.reg);
		else stack.free(v->loc.stack_addr);

		for (u8 i = 0;i < scopes.size();i++) {
			for (u16 vi = 0;vi < scopes[i].vars.size();vi++) {
				var* sv = scopes[i].vars[vi];
				if (sv == v) {
					scopes[i].vars.erase(scopes[i].vars.begin() + vi);
					delete v;
					return;
				}
			}
		}

		// exception
	}

	void func::free_consumed_vars() {
		for (u8 i = 0;i < scopes.size();i++) {
			for (u16 vi = 0;vi < scopes[i].vars.size();vi++) {
				var* v = scopes[i].vars[vi];
				if (!v->is_variable || v->no_auto_free) continue;
				if (v->ref_count == v->total_ref_count) free(v);
			}
		}
	}

	var* func::get(compile_context& ctx, ast_node* identifier) {
		string name = *identifier;
		for (u8 i = 0;i < scopes.size();i++) {
			for (u16 vi = 0;vi < scopes[i].vars.size();vi++) {
				var* v = scopes[i].vars[vi];
				if (v->name != name) continue;
				v->ref_count++;
				return v;
			}
		}

		// exception
		return nullptr;
	}

	void func::push_scope() {
		scopes.push_back(scope());
	}

	void func::pop_scope(compile_context& ctx, ast_node* because) {
		scope& s = scopes.back();
		vector<var*> vars = s.vars;
		for (u32 i = 0;i < vars.size();i++) free(vars[i]);

		for (u16 i = 0;i < s.stack_objects.size();i++) {
			address addr = s.stack_objects[i].first;
			data_type* type = s.stack_objects[i].second;
			if (type->dtor) {
				var tmp;
				tmp.type = type;
				tmp.is_reg = true;
				tmp.loc.reg = vmr::a0;
				tmp.ctx = &ctx;

				ctx.add(
					encode(vmi::addui).operand(vmr::a0).operand(vmr::sp).operand(addr),
					because
				);

				// destruct
				call(ctx, type->dtor, because, { &tmp });
			}
			stack.free(addr);
		}

		scopes.pop_back();
	}

	bool func::in_use(vmr reg) {
		for (u8 i = 0;i < registers.used.size();i++) {
			if (registers.used[i] == reg) return true;
		}
		return false;
	}

	void func::free_stack_object(var* obj, ast_node* because) {
		if (!obj->refers_to_stack_obj) return;

		if (obj->type->dtor) {
			call(*obj->ctx, obj->type->dtor, because, { obj });
		}
		stack.free(obj->refers_to_stack_addr);
		bool found = false;
		for (u8 si = 0;si < scopes.size();si++) {
			scope& s = scopes[si];
			for (u16 oi = 0;oi < s.stack_objects.size();oi++) {
				if (s.stack_objects[oi].first == obj->refers_to_stack_addr) {
					s.stack_objects.erase(s.stack_objects.begin() + oi);
					found = true;
					break;
				}
			}
			if (found) break;
		}
	}

	void func::generate_return_code(compile_context& ctx, ast_node* because, address returns_stack_addr) {
		for (u8 si = 0;si < scopes.size();si++) {
			scope& s = scopes[si];
			for (u16 oi = 0;oi < s.stack_objects.size();oi++) {
				address addr = s.stack_objects[oi].first;
				if (addr == returns_stack_addr) continue;

				data_type* type = s.stack_objects[oi].second;
				if (type->dtor) {
					var tmp;
					tmp.type = type;
					tmp.is_reg = true;
					tmp.loc.reg = vmr::a0;
					tmp.ctx = &ctx;

					ctx.add(
						encode(vmi::addui).operand(vmr::a0).operand(vmr::sp).operand(addr),
						because
					);

					// destruct
					call(ctx, type->dtor, because, { &tmp });
				}

				// don't actually free the stack region, this might not be the end of the function
			}
		}
	}

	var* func::allocate(compile_context& ctx, ast_node* decl, bool is_arg) {
		if (auto_free_consumed_vars) free_consumed_vars();

		data_type* type = ctx.type(decl->data_type);

		vmr reg;
		if (type->is_floating_point) reg = registers.allocate_fp();
		else {
			if (!is_arg) reg = registers.allocate_gp();
			else reg = registers.allocate_arg();
		}

		var* l = new var();
		l->name = *decl->identifier;
		scopes[scopes.size() - 1].vars.push_back(l);
		l->is_variable = true;
		l->ctx = &ctx;
		l->type = type;
		l->count = 1;
		l->is_imm = false;
		l->ref_count = 1; // declaration counts as a ref
		if (reg != vmr::register_count) {
			l->is_reg = true;
			l->loc.reg = reg;
		} else {
			l->is_reg = false;
			l->loc.stack_addr = stack.allocate(type->size);
		}
		return l;
	}

	var* func::allocate(compile_context& ctx, data_type* type, bool is_arg) {
		if (auto_free_consumed_vars) free_consumed_vars();

		vmr reg;
		if (type->is_floating_point) reg = registers.allocate_fp();
		else {
			if (!is_arg) reg = registers.allocate_gp();
			else reg = registers.allocate_arg();
		}

		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = type;
		l->count = 1;
		l->is_imm = false;
		if (reg != vmr::register_count) {
			l->is_reg = true;
			l->loc.reg = reg;
		} else {
			l->is_reg = false;
			l->loc.stack_addr = stack.allocate(type->size);
		}
		return l;
	}

	var* func::allocate(compile_context& ctx, const string& _name, data_type* type, bool is_arg) {
		if (auto_free_consumed_vars) free_consumed_vars();

		vmr reg;
		if (type->is_floating_point) reg = registers.allocate_fp();
		else {
			if (!is_arg) reg = registers.allocate_gp();
			else reg = registers.allocate_arg();
		}

		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = _name;
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = type;
		l->count = 1;
		l->is_imm = false;
		if (reg != vmr::register_count) {
			l->is_reg = true;
			l->loc.reg = reg;
		} else {
			l->is_reg = false;
			l->loc.stack_addr = stack.allocate(type->size);
		}
		return l;
	}

	var* func::allocate_stack_var(compile_context& ctx, data_type* type, ast_node* because) {
		if (auto_free_consumed_vars) free_consumed_vars();

		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->ctx = &ctx;
		l->type = type;
		l->is_reg = true;
		l->refers_to_stack_obj = true;
		l->refers_to_stack_addr = stack.allocate(type->actual_size);
		l->loc.reg = registers.allocate_gp();

		scopes[scopes.size() - 1].stack_objects.push_back(pair<address, data_type*>(l->refers_to_stack_addr, type));
		ctx.add(
			encode(vmi::addui).operand(l->loc.reg).operand(vmr::sp).operand(l->refers_to_stack_addr),
			because
		);
		return l;
	}

	var* func::imm(compile_context& ctx, integer i) {
		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("i64");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->is_const = true;
		l->imm.i = i;
		return l;
	}

	var* func::imm(compile_context& ctx, f32 f) {
		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("f32");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->is_const = true;
		l->imm.f_32 = f;
		return l;
	}

	var* func::imm(compile_context& ctx, f64 d) {
		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("f64");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->is_const = true;
		l->imm.f_64 = d;
		return l;
	}

	var* func::imm(compile_context& ctx, char* s) {
		// todo
		return nullptr;
	}

	var* func::zero(compile_context& ctx, data_type* type) {
		var* l = new var();
		scopes[scopes.size() - 1].vars.push_back(l);
		l->name = format("__anon_%d", anon_var_id++);
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("i32");
		l->count = 1;
		l->is_reg = true;
		l->is_imm = false;
		l->is_const = true;
		l->loc.reg = vmr::zero;
		return l;
	}





	address func::_stack::allocate(u32 size) {
		if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
		address sz = 0;
		for (u32 i = 0;i < slots.size();i++) {
			if (slots[i].size == size && !slots[i].used) {
				slots[i].used = true;
				return slots[i].addr;
			}
			sz += slots[i].size;
		}

		slots.push_back({ sz, size, true });
		return sz;
	}

	void func::_stack::free(address addr) {
		for (u32 i = 0;i < slots.size();i++) {
			if (slots[i].addr == addr) {
				slots[i].used = false;
				return;
			}
		}
	}

	u32 func::_stack::size() {
		u32 sz = 0;
		for (u32 i = 0;i < slots.size();i++) sz += slots[i].size;
		return sz;
	}





	vmr func::_registers::allocate_arg() {
		if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
		if (a.size() == 0) return vmr::register_count;
		vmr r = a.front();
		a.erase(a.begin());
		used.push_back(r);
		return r;
	}

	vmr func::_registers::allocate_gp() {
		if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
		if (gp.size() == 0) return vmr::register_count;
		vmr r = gp.back();
		gp.pop_back();
		used.push_back(r);
		return r;
	}

	vmr func::_registers::allocate_fp() {
		if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
		if (fp.size() == 0) return vmr::register_count;
		vmr r = fp.back();
		fp.pop_back();
		used.push_back(r);
		return r;
	}

	void func::_registers::free(vmr reg) {
		for (u8 i = 0;i < used.size();i++) {
			if (used[i] == reg) {
				used.erase(used.begin() + i);
				break;
			}
		}
		if (reg >= vmr::s0 && reg <= vmr::s7) gp.push_back(reg);
		else if (reg >= vmr::a0 && reg <= vmr::a7) a.push_back(reg);
		else fp.push_back(reg);
	}


	void get_return_value(compile_context& ctx, func* to, ast_node* because, var* ret, data_type* ret_tp) {
		if (to->returns_on_stack) {
			address dest_stack_loc = ctx.cur_func->stack.allocate(ret_tp->actual_size);
			// call memcopy to copy from called function's stack to current stack

			// move $sp + dest_stack_loc to the destination parameter
			ctx.add(
				encode(vmi::addui).operand(vmr::a0).operand(vmr::sp).operand(dest_stack_loc),
				because
			);

			// check if the source == dest, and if so, skip the call to memcopy
			ctx.add(
				encode(vmi::cmp).operand(vmr::v0).operand(vmr::a0).operand(to->return_loc),
				because
			);
			ctx.add(
				encode(vmi::bneqz).operand(vmr::v0).operand(ctx.out->size() + 4),
				because
			);

			// move return_loc to the source parameter
			ctx.add(
				encode(vmi::addui).operand(vmr::a1).operand(to->return_loc).operand(vmr::zero),
				because
			);

			// move size of type to size parameter
			ctx.add(
				encode(vmi::addui).operand(vmr::a2).operand(vmr::zero).operand(ret_tp->actual_size),
				because
			);

			// make the call
			func* memcopy = ctx.function("memcopy");
			ctx.add(
				encode(vmi::jal).operand(memcopy->entry),
				because
			);


			// move the address into return value
			ctx.add(
				encode(vmi::addui).operand(ret->to_reg(because)).operand(vmr::sp).operand(dest_stack_loc),
				because
			);
			ret->refers_to_stack_addr = dest_stack_loc;
			ret->refers_to_stack_obj = true;
			ctx.cur_func->scopes[ctx.cur_func->scopes.size() - 1].stack_objects.push_back(pair<address, data_type*>(dest_stack_loc, ret_tp));
		} else {
			if (to->returns_pointer) {
				if (ret_tp->is_primitive) {
					// load value directly to return var
					vmi ld;
					switch (ret_tp->size) {
						case 1: { ld = vmi::ld8; break; }
						case 2: { ld = vmi::ld16; break; }
						case 4: { ld = vmi::ld32; break; }
						case 8: { ld = vmi::ld64; break; }
					}

					ctx.add(
						encode(ld).operand(ret->loc.reg).operand(to->return_loc),
						because
					);
				} else if (ret->loc.reg != to->return_loc) {
					// it should be a pointer
					ctx.add(
						encode(vmi::add).operand(ret->loc.reg).operand(to->return_loc).operand(vmr::zero),
						because
					);
				}
			} else {
				// if it's not a pointer, it is either a primitive type or an unsupported host stack return
				if (ret->loc.reg != to->return_loc) {
					if (!is_fp(ret->loc.reg)) {
						ctx.add(
							encode(vmi::add).operand(ret->loc.reg).operand(to->return_loc).operand(vmr::zero),
							because
						);
					} else {
						ctx.add(
							encode(vmi::fadd).operand(ret->loc.reg).operand(to->return_loc).operand(vmr::zero),
							because
						);
					}
				}
			}
		}
	}

	var* call(compile_context& ctx, func* to, ast_node* because, const vector<var*>& args, data_type* method_of) {
		if (args.size() != to->args.size()) {
			u32 ac = to->args.size();
			u32 pc = args.size();
			if (to->name.find_first_of("::") != string::npos) {
				// class method, subtract 1 for 'this' obj (since it's passed automatically)
				if (ac > 0) ac--;
				if (pc > 0) pc--;
			}

			if (to->is_subtype_obj_ctor) {
				// subtype object constructor, subtract 1 for type id (since it's passed automatically)
				if (ac > 0) ac--;
				if (pc > 0) pc--;
			}
			ctx.log->err(format(
				"Function '%s' expects %d argument%s, %d %s provided",
				to->name.c_str(), ac, ac == 1 ? "" : "s", pc, pc == 1 ? "was" : "were"
			), because);
		}

		vector<bool> arg_af;
		for (u8 a = 0;a < args.size();a++) {
			arg_af.push_back(args[a]->no_auto_free);
			args[a]->no_auto_free = true;
		}

		// free any used up variables that don't need to be
		// stored in the stack, if it's safe to do so
		if (ctx.cur_func->auto_free_consumed_vars) ctx.cur_func->free_consumed_vars();

		// back up register vars to the stack
		for (u32 s = 0;s < ctx.cur_func->scopes.size();s++) {
			for (u16 vi = 0;vi < ctx.cur_func->scopes[s].vars.size();vi++) {
				var* v = ctx.cur_func->scopes[s].vars[vi];
				if (v->is_reg) v->to_stack(because);
			}
		}

		// if method of subtype class: pass subtype id as second parameter !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		// set arguments
		for (u8 i = 0;i < args.size() && i < to->args.size();i++) {
			data_type* tp = to->args[i].type;
			if (tp->name == "__subtype__") {
				// argument type should be whatever the subtype is
				tp = method_of->sub_type;

				// set $v2 to argument type so the VM knows how to handle the argument
				ctx.add(
					encode(vmi::addui).operand(vmr::v2).operand(vmr::zero).operand((u64)tp->type_id),
					because
				);
			}

			var* a = cast(ctx, args[i], tp, because);
			if (a) {
				if (a != args[i]) args[i]->move_stack_reference(a);
				a->store_in(to->args[i].loc, because);
				if (a != args[i]) ctx.cur_func->free(a);
			}
		}

		// backup $ra
		address ra_addr = ctx.cur_func->stack.allocate(4);
		ctx.add(
			encode(vmi::st32).operand(vmr::ra).operand(vmr::sp).operand(ra_addr),
			because
		);

		uinteger stack_size = ctx.cur_func->stack.size();
		// move stack pointer
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand(stack_size),
				because
			);
		}

		// allocate register for return value if necessary
		var* ret = nullptr;
		if (to->return_type->size > 0) {
			data_type* ret_tp = to->return_type;
			if (to->return_type->name == "__subtype__") ret_tp = method_of->sub_type;
			ret = ctx.cur_func->allocate(ctx, ret_tp);
		}

		// make the call
		ctx.add(
			encode(vmi::jal).operand(to->entry),
			because
		);

		// restore $sp
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::subui).operand(vmr::sp).operand(vmr::sp).operand(stack_size),
				because
			);
		}

		// store return value if necessary
		if (ret) {
			data_type* ret_tp = to->return_type;
			if (to->return_type->name == "__subtype__") ret_tp = method_of->sub_type;
			get_return_value(ctx, to, because, ret, ret_tp);
		}

		// restore $ra
		ctx.cur_func->stack.free(ra_addr);
		ctx.add(
			encode(vmi::ld32).operand(vmr::ra).operand(vmr::sp).operand(ra_addr),
			because
		);

		for (u8 a = 0;a < args.size();a++) args[a]->no_auto_free = arg_af[a];
		ctx.cur_func->free_consumed_vars();

		return ret;
	}
};