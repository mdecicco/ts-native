#include <compiler/compiler.h>
#include <compiler/variable.h>
#include <compiler/function.h>
#include <compiler/data_type.h>
#include <compiler/context.h>
#include <compiler/expression.h>
#include <compiler/compiler.h>

#include <context.h>
#include <instruction_array.h>
#include <register.h>
#include <compile_log.h>
#include <vm_state.h>
#include <robin_hood.h>

using namespace std;

namespace gjs {
	using nt = ast_node::node_type;
	using op = ast_node::operation_type;
	using vmr = vm_register;
	using vmi = vm_instruction;

	#define is_fp(r) (r >= vmr::f0 && r <= vmr::f15)
	#define is_arg(r) (r >= vmr::a0 && r <= vmr::a7)

	compile_exception::compile_exception(const std::string& _file, const string& _text, const string& _lineText, u32 _line, u32 _col) {
		file = _file;
		text = _text;
		lineText = _lineText;
		line = _line;
		col = _col;
	}

	compile_exception::compile_exception(const std::string& text, ast_node* node) :
		compile_exception(node->start.file, text, node->start.lineText, node->start.line, node->start.col)
	{
	}

	compile_exception::~compile_exception() { }


	u16 count_references(ast_node* node, const string& identifier, bool ignoreNext) {
		if (!node) return 0;

		u16 refs = 0;
		if (node->type == nt::identifier) return (string(*node) == identifier) ? 1 : 0;

		if (!ignoreNext) refs += count_references(node->next, identifier);
		refs += count_references(node->data_type, identifier);
		refs += count_references(node->identifier, identifier);
		refs += count_references(node->arguments, identifier);
		refs += count_references(node->body, identifier);
		refs += count_references(node->else_body, identifier);
		refs += count_references(node->property, identifier);
		refs += count_references(node->initializer, identifier);
		refs += count_references(node->lvalue, identifier);
		refs += count_references(node->rvalue, identifier);
		refs += count_references(node->callee, identifier);
		refs += count_references(node->constructor, identifier);
		refs += count_references(node->destructor, identifier);
		refs += count_references(node->condition, identifier);
		refs += count_references(node->modifier, identifier);

		return refs;
	}

	void compile_function(compile_context& ctx, ast_node* node, func* out) {
		out->name = *node->identifier;
		out->return_type = ctx.type(*node->data_type);
		out->entry = ctx.out->size();
		out->root = node;

		ctx.cur_func = out;
		out->push_scope();

		if (out->this_type) {
			var* self = out->allocate(ctx, "this", out->this_type, true);
			self->is_variable = true;
			self->is_const = true;
			self->name = "this";
			self->total_ref_count = count_references(node, "this");
			self->ref_count = 0;
			out->args.push_back({ "this", ctx.type("data"), self->loc.reg });
		}

		ctx.add(encode(vmi::null), node);

		ast_node* arg = node->arguments ? node->arguments->body : nullptr;
		while (arg) {
			var* v = ctx.cur_func->allocate(ctx, arg, true);
			v->total_ref_count = count_references(node, v->name, true);
			out->args.push_back({ *arg->identifier, ctx.type(arg->data_type), v->to_reg(arg) });
			arg = arg->next;
		}

		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			if (out->reached_return) break;
			n = n->next;
		}

		out->pop_scope(ctx, node);

		if (!out->reached_return) {
			if (out->return_type->size == 0) {
				ctx.add(encode(vmi::jmpr).operand(vmr::ra), node);
			} else {
				if (out->is_ctor) {
					out->return_loc = vmr::a0;
					ctx.add(encode(vmi::jmpr).operand(vmr::ra), node);
				} else ctx.log->err(format("Function '%s' must return a value", out->name.c_str()), node);
			}
		}
		ctx.cur_func = nullptr;
	}

	void compile_variable_declaration(compile_context& ctx, ast_node* node) {
		var* var = ctx.cur_func->allocate(ctx, node);
		var->total_ref_count = count_references(ctx.cur_func->root, var->name);

		if (node->initializer) compile_expression(ctx, node->initializer->body, var);
	}

	void compile_delete_statement(compile_context& ctx, ast_node* node) {
		var* del = compile_expression(ctx, node->rvalue, nullptr);
		if (!del) return; // An error was already emitted

		if (del->refers_to_stack_obj) {
			ctx.log->err("Cannot delete an object that was not instantiated with 'new'", node);
			return;
		}

		func* dtor = del->type->dtor;
		vector<var*> args = { del };
		if (!dtor) {
			call(ctx, ctx.function("free"), node, args);
		} else {
			del->no_auto_free = true;
			call(ctx, dtor, node, args);
			del->no_auto_free = false;
			call(ctx, ctx.function("free"), node, args);
		}

		if (!del->is_variable) ctx.cur_func->free(del);
	}

	void compile_if_statement(compile_context& ctx, ast_node* node) {
		var* cond = compile_expression(ctx, node->condition, nullptr);
		address branchAddr = ctx.out->size();
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)),
			node
		);
		ctx.cur_func->free(cond);
		ast_node* n = node->body;
		ctx.cur_func->push_scope();
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		// update branch fail address
		instruction branch = (*ctx.out)[branchAddr];
		ctx.out->set(
			branchAddr,
			encode(branch.instr()).operand(branch.op_1r()).operand(ctx.out->size())
		);

		n = node->else_body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}
		ctx.cur_func->pop_scope(ctx, node);
	}

	void compile_for_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->push_scope();
		ctx.cur_func->auto_free_consumed_vars = false;
		
		if (node->initializer) compile_variable_declaration(ctx, node->initializer);
		address loop_cond = ctx.out->size();
		address branch_addr = 0;
		if (node->condition) {
			var* cond = compile_expression(ctx, node->condition, nullptr);
			branch_addr = ctx.out->size();
			ctx.add(
				encode(vmi::bneqz).operand(cond->to_reg(node->condition)),
				node->condition
			);
			ctx.cur_func->free(cond);
		}

		if (node->modifier) {
			var* mod = compile_expression(ctx, node->modifier, nullptr);
			ctx.cur_func->free(mod);
		}

		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		ctx.add(
			encode(vmi::jmp).operand(loop_cond),
			node
		);

		if (node->condition) {
			// update branch fail address
			instruction branch = (*ctx.out)[branch_addr];
			ctx.out->set(
				branch_addr,
				encode(branch.instr()).operand(branch.op_1r()).operand(ctx.out->size())
			);
		}

		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope(ctx, node);
	}

	void compile_while_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->auto_free_consumed_vars = false;
		address condAddr = ctx.out->size();
		var* cond = compile_expression(ctx, node->condition, nullptr);
		address branchAddr = ctx.out->size();
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)),
			node
		);
		ctx.cur_func->free(cond);
		ast_node* n = node->body;
		ctx.cur_func->push_scope();
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		ctx.add(
			encode(vmi::jmp).operand(condAddr),
			node
		);

		// update branch fail address
		instruction branch = (*ctx.out)[branchAddr];
		ctx.out->set(
			branchAddr,
			encode(branch.instr()).operand(branch.op_1r()).operand(ctx.out->size())
		);

		n = node->else_body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}
		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope(ctx, node);
	}

	void compile_do_while_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->auto_free_consumed_vars = false;
		address startAddr = ctx.out->size();
		ctx.cur_func->push_scope();
		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		var* cond = compile_expression(ctx, node->condition, nullptr);
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)).operand(ctx.out->size() + 1),
			node
		);
		ctx.cur_func->free(cond);

		ctx.add(
			encode(vmi::jmp).operand(startAddr),
			node->condition
		);

		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope(ctx, node);
	}

	void compile_return_statement(compile_context& ctx, ast_node* node) {
		address ret_stack_addr = UINT32_MAX;
		if (node->body) {
			var* ret = compile_expression(ctx, node->body, nullptr);
			if (ret) {
				if (ctx.cur_func->is_ctor) {
					ctx.log->err("Class constructors must not return a value", node);
				} else if (ctx.cur_func->is_dtor) {
					ctx.log->err("Class destructors must not return a value", node);
				} else {
					var* ovar = cast(ctx, ret, ctx.cur_func->return_type, node);
					if (ovar->refers_to_stack_obj) {
						ret_stack_addr = ovar->refers_to_stack_addr;
						if (!ctx.cur_func->return_loc_determined) {
							ctx.cur_func->returns_on_stack = true;
							ctx.cur_func->return_loc = ovar->to_reg(node);
							ctx.cur_func->return_loc_determined = true;
						} else {
							if (!ctx.cur_func->returns_on_stack) {
								ctx.log->err(
									format(
										"Function '%s' has differing return statement values. "
										"At least one return value refers to a dynamically allocated object while at least one does not.",
										ctx.cur_func->name.c_str()
									),
									node
								);
							} else ovar->store_in(ctx.cur_func->return_loc, node);
						}
					} else {
						if (!ctx.cur_func->return_loc_determined) {
							ctx.cur_func->return_loc = ovar->to_reg(node);
							ctx.cur_func->return_loc_determined = true;
						} else {
							if (ctx.cur_func->returns_on_stack) {
								ctx.log->err(
									format(
										"Function '%s' has differing return statement values. "
										"At least one return value refers to a dynamically allocated object while at least one does not.",
										ctx.cur_func->name.c_str()
									),
									node
								);
							}
							else ovar->store_in(ctx.cur_func->return_loc, node);
						}
					}
					if (ovar != ret) ctx.cur_func->free(ovar);
				}
				if (!ret->is_variable) ctx.cur_func->free(ret);
			}
		}

		ctx.cur_func->generate_return_code(ctx, node, ret_stack_addr);

		ctx.add(
			encode(vmi::jmpr).operand(vmr::ra),
			node
		);

		if (ctx.cur_func->scopes.size() == 1) {
			// return in the function's root scope
			ctx.cur_func->reached_return = true;
		}
	}

	void compile_node(compile_context& ctx, ast_node* node) {
		switch(node->type) {
			case nt::empty: break;
			case nt::variable_declaration: { compile_variable_declaration(ctx, node); break; }
			case nt::function_declaration: {
				ctx.funcs.push_back(new func());
				compile_function(ctx, node, ctx.funcs[ctx.funcs.size() - 1]);
				break;
			}
			// this data_type constructor automatically adds it to ctx, it doesn't go nowhere
			case nt::class_declaration: { new data_type(ctx, node); break; }
			case nt::format_declaration: { new data_type(ctx, node); break; }
			case nt::variable_initializer: break;
			case nt::format_property: break;
			case nt::function_arguments: break;
			case nt::if_statement: { compile_if_statement(ctx, node); break; }
			case nt::for_loop: { compile_for_loop(ctx, node); break; }
			case nt::while_loop: { compile_while_loop(ctx, node); break; }
			case nt::do_while_loop: { compile_while_loop(ctx, node); break; }
			case nt::import_statement: break;
			case nt::export_statement: break;
			case nt::return_statement: { compile_return_statement(ctx, node); break; }
			case nt::delete_statement: { compile_delete_statement(ctx, node); break; }
			case nt::object: { break; }
			case nt::call: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp) {
					ctx.cur_func->free_stack_object(tmp, node);
					ctx.cur_func->free(tmp);
				}
				break;
			}
			case nt::expression: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp && !tmp->is_variable) {
					ctx.cur_func->free_stack_object(tmp, node);
					ctx.cur_func->free(tmp);
				}
				break;
			}
			case nt::conditional: { break; }
			case nt::constant: { break; }
			case nt::identifier: { break; }
			case nt::type_identifier: { break; }
			case nt::operation: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp && !tmp->is_variable) {
					ctx.cur_func->free_stack_object(tmp, node);
					ctx.cur_func->free(tmp);
				}
				break;
			}
			case nt::context_function: { break; }
			case nt::context_type: { break; }
			default: {
				throw compile_exception("Invalid AST node type", node);
			}
		}

		if (ctx.cur_func && ctx.cur_func->auto_free_consumed_vars) ctx.cur_func->free_consumed_vars();
	}

	void init_context(compile_context& ctx) {
		ctx.do_store_member_pointer = false;
		ctx.last_member_was_pointer = false;
		ctx.cur_func = nullptr;
		ctx.last_type_method = nullptr;
		data_type* t = nullptr;

		vector<vm_type*> types = ctx.ctx->types()->all();
		for (u32 i = 0;i < types.size();i++) {
			ctx.types.push_back(new data_type(types[i]->name));
			t = ctx.types[ctx.types.size() - 1];
			t->size = t->actual_size = types[i]->size;
			t->type = types[i];
			t->built_in = types[i]->is_builtin;
			t->is_floating_point = types[i]->is_floating_point;
			t->is_unsigned = types[i]->is_unsigned;
			t->is_primitive = types[i]->is_primitive;
			t->accepts_subtype = types[i]->accepts_subtype;
			if (!t->built_in || t->name == "string") {
				// object pointer
				t->size = sizeof(void*);
			}
		}


		vector<vm_function*> funcs = ctx.ctx->all_functions();
		for (u32 i = 0;i < funcs.size();i++) {
			ctx.funcs.push_back(new func(ctx, funcs[i]));
		}

		// type methods and properties must come after functions are added
		for (u32 i = 0;i < types.size();i++) {
			data_type* t = ctx.type(types[i]->name);

			for (u16 p = 0;p < t->type->properties.size();p++) {
				vm_type::property& prop = t->type->properties[p];
				t->props.push_back({
					prop.flags,
					prop.name,
					ctx.type(prop.type->name),
					prop.offset,
					prop.getter ? ctx.function(prop.getter->name) : nullptr,
					prop.setter ? ctx.function(prop.setter->name) : nullptr
				});
			}

			for (u16 m = 0;m < t->type->methods.size();m++) {
				func* f = ctx.function(t->type->methods[m]->name);
				if (f->is_thiscall) f->this_type = t;
				t->methods.push_back(f);
			}

			t->ctor = t->type->constructor ? ctx.function(t->type->constructor->name) : nullptr;
			t->dtor = t->type->destructor ? ctx.function(t->type->destructor->name) : nullptr;
		}
	}

	void compile_ast(vm_context* vctx, ast_node* tree, instruction_array* out, source_map* map, compile_log* log) {
		if (!tree) {
			throw compile_exception("No AST to compile", "", "", 0, 0);
		}

		compile_context ctx;
		ctx.ctx = vctx;
		ctx.map = map;
		ctx.out = out;
		ctx.log = log;
		init_context(ctx);

		ast_node* n = tree->body;
		if (!n) throw compile_exception("AST has no body", tree);
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		if (ctx.log->errors.size() == 0) {
			vector<data_type*> new_types;
			for (u32 i = 0;i < ctx.types.size();i++) {
				data_type* t = ctx.types[i];
				if (t->type) continue; // already in context
				if (t->sub_type) continue; // sub-typed types only exist for the compiler's convenience, don't need to be added

				vm_type* tp = vctx->types()->add(t->name, t->name);
				tp->accepts_subtype = t->accepts_subtype;
				t->type = tp;
				for (u32 p = 0;p < t->props.size();p++) {
					data_type::property& sprop = t->props[p];
					vm_type::property prop;
					prop.name = sprop.name;
					prop.offset = sprop.offset;
					prop.flags = sprop.flags;
					tp->properties.push_back(prop);
				}
				new_types.push_back(t);
			}

			for (u32 i = 0;i < ctx.funcs.size();i++) {
				if (ctx.funcs[i]->bound_to) continue;
				func* sf = ctx.funcs[i];
				vm_function* f = new vm_function(vctx, sf->name, sf->entry);
				for (u8 a = 0;a < sf->args.size();a++) {
					f->signature.arg_types.push_back(sf->args[a].type->type);
					f->signature.arg_locs.push_back(sf->args[a].loc);
				}
				f->signature.returns_on_stack = sf->returns_on_stack;
				f->signature.return_loc = sf->return_loc;
				f->signature.return_type = sf->return_type->type;
				vctx->add(f);
			}

			for (u32 i = 0;i < new_types.size();i++) {
				data_type* t = new_types[i];
				vm_type* tp = t->type;
				for (u32 p = 0;p < t->props.size();p++) {
					data_type::property& sprop = t->props[p];
					vm_type::property& tprop = tp->properties[p];
					tprop.getter = sprop.getter ? vctx->function(sprop.getter->entry) : nullptr;
					tprop.setter = sprop.setter ? vctx->function(sprop.setter->entry) : nullptr;
					tprop.type = sprop.type->type;
				}

				for (u32 m = 0;m < t->methods.size();m++) {
					func* f = t->methods[m];
					tp->methods.push_back(vctx->function(f->entry));
				}

				if (t->ctor) tp->constructor = vctx->function(t->ctor->entry);
				if (t->dtor) tp->destructor = vctx->function(t->dtor->entry);
			}
		}
	}
};