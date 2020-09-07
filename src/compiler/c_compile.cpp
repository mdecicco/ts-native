#include <compiler/compile.h>
#include <compiler/context.h>
#include <compiler/function.h>
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
            ctx.push_node(n->data_type);
            vm_type* tp = ctx.type(n->data_type);
            ctx.pop_node();
            ctx.push_node(n->identifier);
            var v = ctx.empty_var(tp, *n->identifier);
            ctx.pop_node();

            if (n->initializer) {
                v.operator_eq(expression(ctx, n->initializer));
            } else {
                if (!tp->is_primitive) {
                    ctx.push_node(n->data_type);
                    ctx.add(operation::stack_alloc).operand(v).operand(ctx.imm(v.size()));

                    if (v.has_unambiguous_method("constructor", v.type(), {})) {
                        // Default constructor
                        std::vector<var> args = { v };
                        std::vector<vm_type*> arg_types = { v.type() };
                        if (v.type()->sub_type) {
                            // second parameter should be type id. This should
                            // only happen for host calls, script subtype calls
                            // should be compiled as if all occurrences of 'subtype'
                            // are the subtype used by the related instantiation
                            args.push_back(ctx.imm((u64)v.type()->sub_type->id()));
                        }

                        vm_function* f = v.method("construct", v.type(), arg_types);
                        if (f) call(ctx, f, args);
                    } else {
                        // No construction
                    }

                    ctx.pop_node();
                }
            }
        }

        void class_declaration(context& ctx, parse::ast* n) {
        }

        void format_declaration(context& ctx, parse::ast* n) {
        }

        void any(context& ctx, ast* n) {
            switch(n->type) {
                case nt::empty: break;
                case nt::variable_declaration: { variable_declaration(ctx, n); break; }
                case nt::function_declaration: { function_declaration(ctx, n); break; }
                case nt::class_declaration: { class_declaration(ctx, n); break; }
                case nt::format_declaration: { format_declaration(ctx, n); break; }
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