#include <compiler/context.h>
#include <compiler/compile.h>
#include <parser/ast.h>

namespace gjs {
    namespace compile {
        void for_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            if (n->initializer) {
                ctx.push_block();
                variable_declaration(ctx, n->initializer);
            }

            tac_instruction* b = nullptr;
            u64 cond_addr = ctx.code_sz();
            if (n->condition) {
                var cond = expression(ctx, n->condition);
                b = &ctx.add(operation::branch).operand(cond);
            }

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            if (n->modifier) {
                var m = expression(ctx, n->modifier);
            }

            ctx.add(operation::jump).operand(ctx.imm(cond_addr));

            if (b) b->operand(ctx.imm((u64)ctx.code_sz()));

            if (n->initializer) ctx.pop_block();
            ctx.pop_node();
        }

        void while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            
            u64 cond_addr = ctx.code_sz();
            var cond = expression(ctx, n->condition);
            ctx.ensure_code_ref();
            tac_instruction& b = ctx.add(operation::branch).operand(cond);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            ctx.add(operation::jump).operand(ctx.imm(cond_addr));
            b.operand(ctx.imm((u64)ctx.code_sz()));

            ctx.pop_node();
        }
        
        void do_while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);

            u64 start_addr = ctx.code_sz();

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            var cond = expression(ctx, n->condition);
            ctx.ensure_code_ref();
            tac_instruction& b = ctx.add(operation::branch).operand(cond);

            ctx.add(operation::jump).operand(ctx.imm(start_addr));

            b.operand(ctx.imm((u64)ctx.code_sz()));

            ctx.pop_node();
        }
    };
};