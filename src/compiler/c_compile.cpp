#include <compiler/compile.h>
#include <compiler/context.h>
#include <parser/ast.h>
#include <context.h>
#include <vm_type.h>
#include <errors.h>

namespace gjs {
	using exc = error::exception;
	using ec = error::ecode;
	using nt = parse::ast::node_type;
	using namespace parse;

	namespace compile {
		void variable_declaration(context& ctx, parse::ast* n) {
		}

		void class_declaration(context& ctx, parse::ast* n) {
		}

		void format_declaration(context& ctx, parse::ast* n) {
		}

		void any(context& ctx, ast* n) {
			switch(n->type) {
				case nt::empty: break;
				case nt::variable_declaration: { variable_declaration(ctx, n); break; }
				case nt::function_declaration: { function_declaration(ctx, n);break; }
				case nt::class_declaration: { class_declaration(ctx, n);break; }
				case nt::format_declaration: { format_declaration(ctx, n);break; }
				case nt::if_statement: { if_statement(ctx, n); break; }
				case nt::for_loop: { for_loop(ctx, n); break; }
				case nt::while_loop: { while_loop(ctx, n); break; }
				case nt::do_while_loop: { do_while_loop(ctx, n); break; }
				case nt::import_statement: { import_statement(ctx, n); break; }
				case nt::export_statement: { export_statement(ctx, n); break; }
				case nt::return_statement: { return_statement(ctx, n); break; }
				case nt::delete_statement: { delete_statement(ctx, n); break; }
                case nt::scoped_block: { block(ctx, n); break; }
				case nt::object:
				case nt::call:
				case nt::expression:
				case nt::conditional:
				case nt::constant:
				case nt::identifier:
				case nt::operation: { expression(ctx, n); break; }
				default: {
					throw exc(ec::c_invalid_node, n->ref);
				}
			}
		}

		void block(context& ctx, ast* b) {
			ast* n = b->body;
			while (n) {
				any(ctx, n);
				n = n->next;
			}
		}

		void compile(vm_context* env, ast* input) {
			if (!input || !input->body) {
				throw exc(ec::c_no_code, input ? input->ref : source_ref("[unknown]", "", 0, 0));
			}

			context ctx;
			ctx.env = env;
			ctx.input = input;
			ctx.new_types = new type_manager(env);

			ast* n = input->body;
			while (n) {
				any(ctx, n);
				n = n->next;
			}

			// todo: copy types to vm context if no errors
			delete ctx.new_types;

			return;
		}
	};
};