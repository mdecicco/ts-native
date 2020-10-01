#include <compiler/compile.h>
#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/context.h>
#include <common/script_type.h>
#include <common/errors.h>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using nt = parse::ast::node_type;
    using namespace parse;

    namespace compile {
        void construct_on_stack(context& ctx, const var& obj, const std::vector<var>& args) {
            ctx.add(operation::stack_alloc).operand(obj).operand(ctx.imm(obj.size()));

            std::vector<script_type*> arg_types = { ctx.type("data") }; // this obj
            script_type* ret_tp = obj.type();
            if (obj.type()->base_type && obj.type()->is_host) {
                ret_tp = obj.type()->base_type;
                arg_types.push_back(ctx.type("data")); // subtype id
            }

            std::vector<var> c_args = { obj };
            if (obj.type()->sub_type && obj.type()->is_host) {
                // second parameter should be type id. This should
                // only happen for host calls, script subtype calls
                // should be compiled as if all occurrences of 'subtype'
                // are the subtype used by the related instantiation
                c_args.push_back(ctx.imm((u64)obj.type()->sub_type->id()));
            }

            if (args.size() > 0) {
                for (u8 a = 0;a < args.size();a++) {
                    arg_types.push_back(args[a].type());
                    c_args.push_back(args[a]);
                }

                script_function* f = obj.method("constructor", ret_tp, arg_types);
                if (f) call(ctx, f, c_args);
            } else {
                if (obj.has_unambiguous_method("constructor", ret_tp, arg_types)) {
                    // Default constructor
                    script_function* f = obj.method("constructor", ret_tp, arg_types);
                    if (f) call(ctx, f, c_args);
                } else {
                    if (obj.has_any_method("constructor")) {
                        ctx.log()->err(ec::c_no_default_constructor, ctx.node()->ref, obj.type()->name.c_str());
                    } else {
                        // No construction necessary
                    }
                }
            }
        }

        void construct_in_memory(context& ctx, const var& obj, const std::vector<var>& args) {
            obj.operator_eq(
                call(
                    ctx,
                    ctx.function("alloc", ctx.type("data"), { ctx.type("u32") }),
                    { ctx.imm((u64)obj.size()) }
                )
            );

            std::vector<script_type*> arg_types = { ctx.type("data") }; // this obj
            script_type* ret_tp = obj.type();
            if (obj.type()->base_type && obj.type()->is_host) {
                ret_tp = obj.type()->base_type;
                arg_types.push_back(ctx.type("data")); // subtype id
            }

            std::vector<var> c_args = { obj };
            if (obj.type()->sub_type && obj.type()->is_host) {
                // second parameter should be type id. This should
                // only happen for host calls, script subtype calls
                // should be compiled as if all occurrences of 'subtype'
                // are the subtype used by the related instantiation
                c_args.push_back(ctx.imm((u64)obj.type()->sub_type->id()));
            }

            if (args.size() > 0) {
                for (u8 a = 0;a < args.size();a++) {
                    arg_types.push_back(args[a].type());
                    c_args.push_back(args[a]);
                }

                script_function* f = obj.method("constructor", ret_tp, arg_types);
                if (f) call(ctx, f, c_args);
            } else {
                if (obj.has_unambiguous_method("constructor", ret_tp, arg_types)) {
                    // Default constructor
                    script_function* f = obj.method("constructor", ret_tp, arg_types);
                    if (f) call(ctx, f, c_args);
                } else {
                    if (obj.has_any_method("constructor")) {
                        ctx.log()->err(ec::c_no_default_constructor, ctx.node()->ref, obj.type()->name.c_str());
                    } else {
                        // No construction necessary
                    }
                }
            }
        }

        void variable_declaration(context& ctx, parse::ast* n) {
            ctx.push_node(n->data_type);
            script_type* tp = ctx.type(n->data_type);
            ctx.pop_node();
            ctx.push_node(n->identifier);
            var v = ctx.empty_var(tp, *n->identifier);
            ctx.pop_node();

            if (n->initializer) {
                v.operator_eq(expression(ctx, n->initializer->body));
            } else {
                if (!tp->is_primitive) {
                    ctx.push_node(n->data_type);
                    construct_on_stack(ctx, v, {});
                    v.raise_stack_flag();
                    ctx.pop_node();
                }
            }

            if (n->is_const) v.raise_flag(bind::pf_read_only);
            if (n->is_static) v.raise_flag(bind::pf_static);
        }

        void class_declaration(context& ctx, parse::ast* n) {
            // ctx.out.types.push_back(...);
        }

        void format_declaration(context& ctx, parse::ast* n) {
            // ctx.out.types.push_back(...);
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
            ctx.push_block();
            ast* n = b->body;
            while (n) {
                any(ctx, n);
                n = n->next;
            }
            ctx.pop_block();
        }

        void compile(script_context* env, ast* input, compilation_output& out) {
            if (!input || !input->body) {
                throw exc(ec::c_no_code, input ? input->ref : source_ref("[unknown]", "", 0, 0));
            }

            context ctx(out);
            ctx.env = env;
            ctx.input = input;
            ctx.new_types = new type_manager(env);
            ctx.push_node(input->body);

            ast* n = input->body;
            try {
                while (n) {
                    any(ctx, n);
                    n = n->next;
                }
            } catch (const error::exception &e) {
                delete ctx.new_types;
                for (u32 i = 0;i < ctx.new_functions.size();i++) delete ctx.new_functions[i];
                throw e;
            } catch (const std::exception &e) {
                delete ctx.new_types;
                for (u32 i = 0;i < ctx.new_functions.size();i++) delete ctx.new_functions[i];
                throw e;
            }

            ctx.pop_node();

            if (ctx.log()->errors.size() > 0) {
                delete ctx.new_types;
                
                for (u32 i = 0;i < ctx.new_functions.size();i++) {
                    delete ctx.new_functions[i];
                }

                throw exc(ec::c_compile_finished_with_errors, input->ref);
            }

            // todo: copy types to vm context if no errors
            delete ctx.new_types;
            return;
        }
    };
};