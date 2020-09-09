#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <vm_type.h>
#include <vm_function.h>
#include <compile_log.h>
#include <errors.h>

namespace gjs {
    using ec = error::ecode;
    namespace compile {
        void import_statement(context& ctx, parse::ast* n) {
        }

        void export_statement(context& ctx, parse::ast* n) {
        }

        void return_statement(context& ctx, parse::ast* n) {
            vm_function* f = ctx.current_function();
            if (!f) {
                ctx.log()->err(ec::c_no_global_return, n->ref);
                return;
            }

            ctx.push_node(n);
            
            if (n->body) {
                if (f->signature.return_type->name == "void") {
                    ctx.log()->err(ec::c_no_void_return_val, n->ref);
                } else {
                    var rv = expression(ctx, n->body).convert(f->signature.return_type);
                    ctx.add(operation::ret).operand(rv);
                }
            } else {
                ctx.add(operation::ret);
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

            vm_function* free = ctx.function("free", ctx.type("void"), { ctx.type("data") });
            call(ctx, free, { obj });

            ctx.pop_node();
        }

        void if_statement(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            tac_instruction& branch = ctx.add(operation::branch).operand(expression(ctx, n->condition));

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            if (n->else_body) {
                tac_instruction& jmp = ctx.add(operation::jump); // jump past else block if the condition evaluates to true, after executing the truth condition
                branch.operand(ctx.imm((u64)ctx.code.size())); // jump to here from branch if the condition evaluates to false
                ctx.push_block();
                any(ctx, n->else_body);
                ctx.pop_block();
                jmp.operand(ctx.imm((u64)ctx.code.size()));
            } else {
                branch.operand(ctx.imm((u64)ctx.code.size()));
            }
            ctx.pop_node();
        }
    };
};