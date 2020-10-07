#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/compile_log.h>
#include <common/errors.h>

namespace gjs {
    using ec = error::ecode;
    namespace compile {
        void import_statement(context& ctx, parse::ast* n) {
        }

        void export_statement(context& ctx, parse::ast* n) {
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
            ctx.ensure_code_ref();
            var cond = expression(ctx, n->condition);
            tac_instruction& branch = ctx.add(operation::branch).operand(cond);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            if (n->else_body) {
                ctx.ensure_code_ref();
                tac_instruction& jmp = ctx.add(operation::jump); // jump past else block if the condition evaluates to true, after executing the truth condition
                branch.operand(ctx.imm((u64)ctx.code_sz())); // jump to here from branch if the condition evaluates to false
                ctx.push_block();
                any(ctx, n->else_body);
                ctx.pop_block();
                jmp.operand(ctx.imm((u64)ctx.code_sz()));
            } else {
                branch.operand(ctx.imm((u64)ctx.code_sz()));
            }
            ctx.pop_node();
        }
    };
};