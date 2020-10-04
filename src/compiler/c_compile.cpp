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

            std::vector<script_type*> arg_types = { obj.type() }; // this obj
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

            std::vector<script_type*> arg_types = { obj.type() }; // this obj
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

        script_type* class_declaration(context& ctx, parse::ast* n) {
            if (ctx.identifier_in_use(*n->identifier) && !(n->is_subtype && ctx.subtype_replacement)) {
                ctx.log()->err(ec::c_identifier_in_use, n->ref, std::string(*n->identifier).c_str());
                return nullptr;
            }

            if (n->is_subtype && !ctx.subtype_replacement) {
                // Will be compiled later for each instantiation of the class
                // with a unique subtype
                ctx.subtype_types.push_back(n);
                return nullptr;
            }

            std::string name = n->is_subtype ? std::string(*n->identifier) + "<" + ctx.subtype_replacement->name + ">" : *n->identifier;

            if (script_type* existing = ctx.new_types->get(name)) {
                class_definition(ctx, n, ctx.subtype_replacement);
                return existing;
            }

            ctx.push_node(n);
            script_type* tp = ctx.new_types->add(name, name);

            std::vector<parse::ast*> methods;
            parse::ast* cn = n->body;
            while (cn) {
                switch (cn->type) {
                    case nt::function_declaration: {
                        methods.push_back(cn);
                        break;
                    }
                    case nt::class_property: {
                        std::string name = *cn->identifier;
                        script_type* p_tp = ctx.type(cn->data_type);
                        u8 flags = 0;
                        if (cn->is_static) flags |= bind::pf_static;
                        if (cn->is_const) flags |= bind::pf_read_only;

                        tp->properties.push_back({ flags, name, p_tp, tp->size, nullptr, nullptr });

                        tp->size += p_tp->size;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                cn = cn->next;
            }

            if (n->destructor) {
                // class methods must be compiled after the class is fully declared
                // (and in the case of subtypes, which are compiled within functions,
                // after the function that instantiates the class with a new subtype
                // is finished compiling)
                parse::ast* body = n->destructor->body;
                n->destructor->body = nullptr;

                tp->destructor = function_declaration(ctx, n->destructor);

                n->destructor->body = body;
            }

            for (u16 i = 0;i < methods.size();i++) {
                // class methods must be compiled after the class is fully declared
                // (and in the case of subtypes, which are compiled within functions,
                // after the function that instantiates the class with a new subtype
                // is finished compiling)
                parse::ast* body = methods[i]->body;
                methods[i]->body = nullptr;

                tp->methods.push_back(function_declaration(ctx, methods[i]));

                methods[i]->body = body;
            }

            ctx.pop_node();

            ctx.deferred.push_back({ n, ctx.subtype_replacement });

            return tp;
        }

        void class_definition(context& ctx, parse::ast* n, script_type* subtype) {
            ctx.push_node(n);
            ctx.subtype_replacement = subtype;

            if (n->destructor) function_declaration(ctx, n->destructor);

            parse::ast* cn = n->body;
            while (cn) {
                switch (cn->type) {
                    case nt::function_declaration: {
                        function_declaration(ctx, cn);
                        break;
                    }
                    default: {
                        break;
                    }
                }
                cn = cn->next;
            }

            ctx.subtype_replacement = nullptr;
            ctx.pop_node();
        }

        script_type* format_declaration(context& ctx, parse::ast* n) {
            if (ctx.identifier_in_use(*n->identifier)) {
                ctx.log()->err(ec::c_identifier_in_use, n->ref, std::string(*n->identifier).c_str());
                return nullptr;
            }

            if (n->is_subtype && !ctx.subtype_replacement) {
                // Will be compiled later for each instantiation of the class
                // with a unique subtype
                ctx.subtype_types.push_back(n);
                return nullptr;
            }

            ctx.push_node(n);
            std::string name = n->is_subtype ? std::string(*n->identifier) + "<" + ctx.subtype_replacement->name + ">" : *n->identifier;

            script_type* tp = ctx.new_types->add(name, name);

            parse::ast* cn = n->body;
            while (cn) {
                switch (cn->type) {
                    case nt::format_property: {
                        std::string name = *cn->identifier;
                        script_type* p_tp = ctx.type(cn->data_type);
                        u8 flags = 0;
                        if (cn->is_static) flags |= bind::pf_static;
                        if (cn->is_const) flags |= bind::pf_read_only;

                        tp->properties.push_back({ flags, name, p_tp, tp->size, nullptr, nullptr });

                        tp->size += p_tp->size;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                cn = cn->next;
            }
            ctx.pop_node();

            return tp;
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

                for (u32 i = 0;i < ctx.deferred.size();i++) {
                    ctx.subtype_replacement = ctx.deferred[i].subtype_replacement;
                    any(ctx, ctx.deferred[i].node);
                    ctx.subtype_replacement = nullptr;
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

            env->types()->merge(ctx.new_types);
             
            delete ctx.new_types;
            return;
        }
    };
};