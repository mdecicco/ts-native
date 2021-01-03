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
            std::string as = n->identifier ? std::string(*n->identifier) : "";

            context::import* im = new context::import;
            im->mod = ctx.env->module(from);
            im->alias = as;
            
            if (n->body->next) {
                // specific symbols
                parse::ast* i = n->body->next;
                while (i) {
                    std::string symbol = *i;
                    std::string alias = i->identifier ? std::string(*i->identifier) : "";
                    script_type* tp = im->mod->types()->get(hash(symbol));
                    script_enum* en = im->mod->get_enum(symbol);

                    if (tp) {
                        im->symbols.push_back({ symbol, alias, tp, nullptr, false, false, true, false });
                    } else if (im->mod->has_local(symbol)) {
                        auto& l = im->mod->local(symbol);
                        im->symbols.push_back({ symbol, alias, l.type, nullptr, true, false, false, false });
                    } else if (im->mod->has_function(symbol)) {
                        im->symbols.push_back({ symbol, alias, nullptr, nullptr, false, true, false, false });
                    } else if (en) {
                        im->symbols.push_back({ symbol, alias, nullptr, en, false, false, false, true });
                    } else {
                        ctx.log()->err(ec::c_symbol_not_found_in_module, i->ref, symbol.c_str(), from.c_str());
                    }

                    i = i->next;
                }
            } else {
                // all symbols
                const auto& types = im->mod->types()->all();
                for (u16 i = 0;i < types.size();i++) im->symbols.push_back({ types[i]->name, "", types[i], nullptr, false, false, true, false });

                auto functions = im->mod->function_names();
                for (u16 i = 0;i < functions.size();i++) im->symbols.push_back({ functions[i], "", nullptr, nullptr, false, true, false, false });

                const auto& locals = im->mod->locals();
                for (u16 i = 0;i < locals.size();i++) im->symbols.push_back({ locals[i].name, "", locals[i].type, nullptr, true, false, false, false });

                const auto& enums = im->mod->enums();
                for (u16 i = 0;i < enums.size();i++) im->symbols.push_back({ enums[i]->name(), "", nullptr, enums[i], false, false, false, true });
            }

            ctx.imports.push_back(im);
        }

        void return_statement(context& ctx, parse::ast* n) {
            script_function* f = ctx.current_function();
            if (!f) {
                f = ctx.out.funcs[0].func;
                
                if (n->body) {
                    if (f->signature.return_type) {
                        var rv = expression(ctx, n->body).convert(f->signature.return_type);
                        ctx.add(operation::ret).operand(rv);
                    } else {
                        var rv = expression(ctx, n->body);
                        ctx.add(operation::ret).operand(rv);
                        f->signature.return_type = rv.type();
                    }
                } else if (!f->signature.return_type) {
                    ctx.add(operation::ret);
                    f->signature.return_type = ctx.type("void");
                } else if (f->signature.return_type->size == 0) ctx.add(operation::ret);
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
                } else if (f->signature.return_type->size == 0) {
                    ctx.log()->err(ec::c_no_void_return_val, n->ref);
                } else {
                    // todo: handle returning stack objects
                    var rv = expression(ctx, n->body).convert(f->signature.return_type);
                    ctx.add(operation::ret).operand(rv);
                }
            } else {
                script_type* this_tp = ctx.class_tp();
                if (this_tp && f->name == this_tp->name + "::constructor") {
                    ctx.add(operation::ret).operand(ctx.get_var("this"));
                } else if (f->signature.return_type->size == 0) ctx.add(operation::ret);
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
                call(ctx, obj.type()->destructor, { obj });
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
                branch.operand(ctx.imm((u64)ctx.code_sz()));
                ctx.push_block();
                any(ctx, n->else_body);
                ctx.pop_block();

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