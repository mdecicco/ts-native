#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/compile_log.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/common/script_enum.h>

namespace gjs {
    using ec = error::ecode;
    namespace compile {
        void import_statement(context& ctx, parse::ast* n) {
            std::string from = *n->body;

            script_module* mod = ctx.env->module(from);
            mod->init();
            
            if (n->body->next) {
                // specific symbols
                parse::ast* i = n->body->next;
                while (i) {
                    std::string symbol = *i;
                    std::string name = i->identifier ? std::string(*i->identifier) : symbol;
                    script_type* tp = mod->types()->get(hash(symbol));
                    script_enum* en = mod->get_enum(symbol);

                    if (tp) ctx.symbols.set(name, tp);
                    else if (mod->has_local(symbol)) ctx.symbols.set(name, &mod->local_info(symbol));
                    else if (en) ctx.symbols.set(name, en);
                    else if (mod->has_function(symbol)) {
                        auto funcs = mod->function_overloads(symbol);
                        u32 c = 0;
                        for (u32 f = 0;f < funcs.size();f++) {
                            if (funcs[f]->is_method_of) continue;
                            c++;
                            ctx.symbols.set(name, funcs[f]);
                        }
                        if (!c) ctx.log()->err(ec::c_symbol_not_found_in_module, i->ref, symbol.c_str(), from.c_str());
                    } else {
                        ctx.log()->err(ec::c_symbol_not_found_in_module, i->ref, symbol.c_str(), from.c_str());
                    }

                    i = i->next;
                }
            } else {
                // all symbols
                std::string name = n->identifier ? std::string(*n->identifier) : from;
                symbol_table* st = ctx.symbols.nest(name);
                st->module = mod;

                const auto& types = mod->types()->all();
                for (u16 i = 0;i < types.size();i++) st->set(types[i]->name, types[i]);

                const auto& locals = mod->locals();
                for (u16 i = 0;i < locals.size();i++) st->set(locals[i].name, &locals[i]);

                const auto& enums = mod->enums();
                for (u16 i = 0;i < enums.size();i++) st->set(enums[i]->name(), enums[i]);

                auto functions = mod->functions();
                for (u16 i = 0;i < functions.size();i++) st->set(functions[i]->name, functions[i]);
            }
        }

        void return_statement(context& ctx, parse::ast* n) {
            script_function* f = ctx.current_function();

            if (f->is_method_of && f == f->is_method_of->destructor) {
                add_implicit_destructor_code(ctx, n, f->is_method_of);
            }

            if (!f) {
                f = ctx.out.funcs[0].func;
                
                if (n->body) {
                    if (f->type) {
                        var rv = expression(ctx, n->body).convert(f->type->signature->return_type);
                        ctx.add(operation::ret).operand(rv);
                    } else {
                        var rv = expression(ctx, n->body);
                        ctx.add(operation::ret).operand(rv);
                        f->update_signature(
                            ctx.env->types()->get(function_signature(ctx.env, rv.type(), false, nullptr, 0, nullptr))
                        );
                    }
                } else if (!f->type) {
                    ctx.add(operation::ret);
                    f->update_signature(
                        ctx.env->types()->get(function_signature(ctx.env, ctx.type("void"), false, nullptr, 0, nullptr))
                    );
                } else if (f->type->signature->return_type->size == 0) ctx.add(operation::ret);
                else ctx.log()->err(ec::c_missing_return_value, n->ref, f->name.c_str());

                return;
            }

            ctx.push_node(n);
            
            if (n->body) {
                script_type* this_tp = ctx.class_tp();
                if (this_tp && f->name == this_tp->name + "::constructor") {
                    ctx.log()->err(ec::c_no_ctor_return_val, n->ref);
                } else if (this_tp && f->name == this_tp->name + "::destructor") {
                    ctx.log()->err(ec::c_no_dtor_return_val, n->ref);
                } else if (f->type->signature->return_type->size == 0) {
                    ctx.log()->err(ec::c_no_void_return_val, n->ref);
                } else {
                    var rv = expression(ctx, n->body).convert(f->type->signature->return_type, true);
                    if (f->type->signature->returns_on_stack) {
                        var& ret_ptr = ctx.get_var("@ret");
                        if (rv.type()->is_primitive) {
                            ctx.add(operation::store).operand(ret_ptr).operand(rv);
                            ctx.add(operation::ret);
                        } else {
                            construct_in_place(ctx, ret_ptr, { rv });
                            ctx.add(operation::ret);
                        }
                    } else ctx.add(operation::ret).operand(rv);
                }
            } else {
                if (f->type->signature->return_type->size == 0) ctx.add(operation::ret);
                else ctx.log()->err(ec::c_missing_return_value, n->ref, f->name.c_str());
            }

            ctx.pop_node();
        }

        void delete_statement(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            var obj = expression(ctx, n->rvalue);
            if (obj.is_stack_obj()) {
                if (obj.name().length() > 0) ctx.log()->err(ec::c_no_stack_delete_named_var, n->ref, obj.name().c_str());
                else ctx.log()->err(ec::c_no_stack_delete, n->ref);
                ctx.pop_node();
                return;
            }
            if (obj.type()->is_primitive) {
                if (obj.name().length() > 0) ctx.log()->err(ec::c_no_primitive_delete_named_var, n->ref, obj.name().c_str());
                else ctx.log()->err(ec::c_no_primitive_delete, n->ref);
                ctx.pop_node();
                return;
            }

            if (obj.type()->destructor) {
                call(ctx, obj.type()->destructor, {}, &obj);
            }

            script_function* free = ctx.function("free", ctx.type("void"), { ctx.type("data") });
            call(ctx, free, { obj });

            ctx.pop_node();
        }

        void if_statement(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            var cond = expression(ctx, n->condition);
            auto meta = ctx.add(operation::meta_if_branch);
            auto branch = ctx.add(operation::branch).operand(cond);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            // truth body end
            meta.operand(ctx.imm((u64)ctx.code_sz()));
            auto jmp = ctx.add(operation::jump);

            if (n->else_body) {
                ctx.push_node(n->else_body);
                branch.operand(ctx.imm((u64)ctx.code_sz()));
                ctx.push_block();
                any(ctx, n->else_body);
                ctx.pop_block();
                ctx.pop_node();

                // false body end
                meta.operand(ctx.imm((u64)ctx.code_sz()));
                ctx.add(operation::jump).operand(ctx.imm((u64)ctx.code_sz()));
            } else {
                // no false body
                meta.operand(ctx.imm((u64)0));
                branch.operand(ctx.imm((u64)ctx.code_sz()));
            }
            jmp.operand(ctx.imm((u64)ctx.code_sz()));

            // join address
            meta.operand(ctx.imm((u64)ctx.code_sz()));
            ctx.pop_node();
        }
    };
};