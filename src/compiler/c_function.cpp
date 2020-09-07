#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <compile_log.h>
#include <errors.h>
#include <vm_function.h>
#include <vm_type.h>
#include <register.h>

namespace gjs {
	using namespace parse;
	using ec = error::ecode;

	namespace compile {
		void function_declaration(context& ctx, ast* n) {
			ctx.push_node(n);
			ctx.push_node(n->data_type);
			vm_type* ret = ctx.type(*n->data_type);
			ctx.pop_node();
			std::vector<vm_type*> arg_types;
			std::vector<std::string> arg_names;
			ast* a = n->arguments->body;
			while (a) {
				ctx.push_node(a->data_type);
				arg_types.push_back(ctx.type(*a->data_type));
				ctx.pop_node();

				ctx.push_node(a->identifier);
				arg_names.push_back(*a->identifier);
				ctx.pop_node();

				a = a->next;
			}

			if (!n->body) {
				// forward declaration
				vm_function* f = ctx.find_func(*n->identifier, ret, arg_types);
				if (f) {
					ctx.log()->err(ec::c_function_already_declared, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
					return;
				}

				ctx.new_functions.push_back(new vm_function(ctx.env, *n->identifier, 0));
				return;
			}
			
			vm_function* f = ctx.find_func(*n->identifier, ret, arg_types);
			if (f) {
				if (f->is_host || f->access.entry) {
					ctx.log()->err(ec::c_function_already_defined, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
					return;
				}
			} else {
				f = new vm_function(ctx.env, *n->identifier, 0);
				u16 gp = (u16)vm_register::a0;
				u16 fp = (u16)vm_register::f0;
				for (u8 i = 0;i < arg_types.size();i++) {
					f->arg(arg_types[i]);
					f->signature.arg_locs.push_back((vm_register)(arg_types[i]->is_floating_point ? fp++ : gp++));
				}
			}
			
			f->access.entry = ctx.code.size();

			block(ctx, n->body);

			ctx.pop_node();
		}

		var function_call(context& ctx, ast* n) {
			return ctx.null_var();
		}

		std::string arg_tp_str(const std::vector<vm_type*> types) {
			std::string o = "(";
			for (u8 i = 0;i < types.size();i++) {
				if (i > 0) o += ", ";
				o += types[i]->name;
			}
			o += ")";

			return o;
		}

		var call(context& ctx, vm_function* func, const std::vector<var>& args) {
			return ctx.dummy_var(func->signature.return_type);
		}

	};
};