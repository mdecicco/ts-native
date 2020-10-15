#include <compiler/compile.h>
#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/script_context.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/errors.h>
#include <common/script_module.h>
#include <vm/register.h>
#include <builtin/script_buffer.h>

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
                // second parameter should be moduletype id. This should
                // only happen for host calls, script subtype calls
                // should be compiled as if all occurrences of 'subtype'
                // are the subtype used by the related instantiation
                u64 moduletype = join_u32(obj.type()->owner->id(), obj.type()->sub_type->id());
                c_args.push_back(ctx.imm(moduletype));
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
                u64 moduletype = join_u32(obj.type()->owner->id(), obj.type()->sub_type->id());
                c_args.push_back(ctx.imm(moduletype));
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

        u64 construct_in_module_memory(context& ctx, const var& obj, const std::vector<var>& args) {
            u64 offset = ctx.out.mod->data()->position();
            ctx.add(operation::module_data).operand(obj).operand(ctx.imm((u64)ctx.out.mod->id())).operand(ctx.imm(offset));

            std::vector<script_type*> arg_types = { obj.type() }; // this obj
            script_type* ret_tp = obj.type();
            if (obj.type()->base_type && obj.type()->is_host) {
                ret_tp = obj.type()->base_type;
                arg_types.push_back(ctx.type("data")); // subtype id
            }
            ctx.out.mod->data()->position(offset + ret_tp->size);

            std::vector<var> c_args = { obj };
            if (obj.type()->sub_type && obj.type()->is_host) {
                // second parameter should be type id. This should
                // only happen for host calls, script subtype calls
                // should be compiled as if all occurrences of 'subtype'
                // are the subtype used by the related instantiation
                u64 moduletype = join_u32(obj.type()->owner->id(), obj.type()->sub_type->id());
                c_args.push_back(ctx.imm(moduletype));
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

            return offset;
        }

        void variable_declaration(context& ctx, parse::ast* n) {
            ctx.push_node(n->data_type);
            script_type* tp = ctx.type(n->data_type);
            ctx.pop_node();
            ctx.push_node(n->identifier);
            var& v = ctx.empty_var(tp, *n->identifier);
            ctx.pop_node();

            if (!ctx.compiling_function) {
                if (n->initializer) {
                    if (!tp->is_primitive) {
                        ctx.push_node(n->data_type);
                        u64 off = construct_in_module_memory(ctx, v, { expression(ctx, n->initializer->body) });
                        ctx.pop_node();
                        ctx.out.mod->define_local(*n->identifier, off, tp, n->data_type->ref);
                    } else {
                        u64 off = ctx.out.mod->data()->position();
                        ctx.out.mod->define_local(*n->identifier, off, tp, n->data_type->ref);
                        ctx.out.mod->data()->position(off + tp->size);

                        var ptr = ctx.empty_var(ctx.type("data"));
                        ctx.add(operation::module_data).operand(ptr).operand(ctx.imm((u64)ctx.out.mod->id())).operand(ctx.imm(off));
                        v.set_mem_ptr(ptr);

                        var val = expression(ctx, n->initializer->body);
                        v.operator_eq(val);
                    }
                } else {
                    if (!tp->is_primitive) {
                        ctx.push_node(n->data_type);
                        u64 off = construct_in_module_memory(ctx, v, {});
                        ctx.pop_node();
                        ctx.out.mod->define_local(*n->identifier, off, tp, n->data_type->ref);
                    } else {
                        u64 off = ctx.out.mod->data()->position();
                        ctx.out.mod->define_local(*n->identifier, off, tp, n->data_type->ref);
                        ctx.out.mod->data()->position(off + tp->size);

                        var ptr = ctx.empty_var(ctx.type("data"));
                        ctx.add(operation::module_data).operand(ptr).operand(ctx.imm((u64)ctx.out.mod->id())).operand(ctx.imm(off));
                        v.set_mem_ptr(ptr);
                    }
                }
            } else {
                if (n->initializer) {
                    if (!tp->is_primitive) {
                        ctx.push_node(n->data_type);
                        v.raise_stack_flag();
                        construct_on_stack(ctx, v, { expression(ctx, n->initializer->body) });
                        ctx.pop_node();
                    } else {
                        var val = expression(ctx, n->initializer->body);
                        v.operator_eq(val);
                    }
                } else {
                    if (!tp->is_primitive) {
                        ctx.push_node(n->data_type);
                        v.raise_stack_flag();
                        construct_on_stack(ctx, v, {});
                        ctx.pop_node();
                    }
                }
            }

            if (n->is_const) v.raise_flag(bind::pf_read_only);
            if (n->is_static) v.raise_flag(bind::pf_static);
        }

        script_type* class_declaration(context& ctx, parse::ast* n) {
            if (ctx.type(*n->identifier, false) && !ctx.compiling_deferred) {
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
            tp->owner = ctx.out.mod;

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
            if (ctx.type(*n->identifier, false) && !ctx.compiling_deferred) {
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
            tp->owner = ctx.out.mod;

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

        void import_globals(context& ctx) {
            context::import* im = new context::import;
            im->mod = ctx.env->global();
            im->alias = "";

            const auto& types = im->mod->types()->all();
            for (u16 i = 0;i < types.size();i++) im->symbols.push_back({ types[i]->name, "", types[i], false, false, true });

            auto functions = im->mod->function_names();
            for (u16 i = 0;i < functions.size();i++) im->symbols.push_back({ functions[i], "", nullptr, false, true, false });

            const auto& locals = im->mod->locals();
            for (u16 i = 0;i < locals.size();i++) im->symbols.push_back({ locals[i].name, "", locals[i].type, true, false, false });
            
            ctx.imports.push_back(im);
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

            import_globals(ctx);

            // return type will be determined by the first global return statement, or it will be void
            script_function* init = new script_function(ctx.env, "__init__", 0);
            init->signature.return_loc = vm_register::v0;
            init->owner = out.mod;
            ctx.new_functions.push_back(init);
            ctx.out.funcs.push_back({ init, gjs::func_stack(), 0, 0, register_allocator(out) });
            ctx.push_block();

            ast* n = input->body;
            try {
                while (n) {
                    any(ctx, n);
                    n = n->next;
                }

                ctx.compiling_deferred = true;
                for (u32 i = 0;i < ctx.deferred.size();i++) {
                    ctx.subtype_replacement = ctx.deferred[i].subtype_replacement;
                    any(ctx, ctx.deferred[i].node);
                    ctx.subtype_replacement = nullptr;
                }

                ctx.compiling_deferred = false;
            } catch (const error::exception &e) {
                delete ctx.new_types;
                for (u32 i = 0;i < ctx.new_functions.size();i++) delete ctx.new_functions[i];
                ctx.out.funcs.clear();
                throw e;
            } catch (const std::exception &e) {
                delete ctx.new_types;
                for (u32 i = 0;i < ctx.new_functions.size();i++) delete ctx.new_functions[i];
                ctx.out.funcs.clear();
                throw e;
            }

            ctx.out.funcs[0].end = ctx.code_sz();
            if (!init->signature.return_type) {
                if (ctx.global_code.size() != 0) {
                    init->signature.return_type = ctx.type("void");
                    ctx.add(operation::ret);
                } else {
                    delete init;
                    ctx.new_functions[0] = nullptr;
                    ctx.out.funcs[0].func = nullptr;
                }
            }
            else if (ctx.global_code.back().op != operation::ret) {
                if (init->signature.return_type->size == 0) ctx.add(operation::ret);
                else ctx.log()->err(ec::c_missing_return_value, n->ref, "__init__");
            }

            ctx.pop_block();
            ctx.pop_node();

            if (ctx.log()->errors.size() > 0) {
                delete ctx.new_types;
                
                for (u32 i = 0;i < ctx.new_functions.size();i++) {
                    if (ctx.new_functions[i]) delete ctx.new_functions[i];
                }

                ctx.out.funcs.clear();

                throw exc(ec::c_compile_finished_with_errors, input->ref);
            }

            out.mod->types()->merge(ctx.new_types);
             
            delete ctx.new_types;
            return;
        }
    };
};