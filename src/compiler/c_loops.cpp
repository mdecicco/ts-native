#include <compiler/context.h>
#include <compiler/compile.h>
#include <parser/ast.h>

namespace gjs {
    namespace compile {
        void for_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            if (n->initializer) {
                if (n->initializer->type == parse::ast::node_type::variable_declaration) {
                    ctx.push_block();
                    variable_declaration(ctx, n->initializer);
                } else {
                    var expr = expression(ctx, n->initializer);
                }
            }

            tac_wrapper b;
            auto meta = ctx.add(operation::meta_for_loop);
            u64 cond_addr = ctx.code_sz();
            if (n->condition) {
                var cond = expression(ctx, n->condition);
                meta.operand(ctx.imm((u64)ctx.code_sz()));
                b = ctx.add(operation::branch).operand(cond);
            }

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            if (n->modifier) {
                var m = expression(ctx, n->modifier);
            }

            meta.operand(ctx.imm((u64)ctx.code_sz()));
            ctx.add(operation::jump).operand(ctx.imm(cond_addr));

            if (b) b.operand(ctx.imm((u64)ctx.code_sz()));

            if (n->initializer && n->initializer->type == parse::ast::node_type::variable_declaration) ctx.pop_block();
            ctx.pop_node();
        }

        void while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            auto m = ctx.add(operation::meta_while_loop);
            u64 cond_addr = ctx.code_sz();
            var cond = expression(ctx, n->condition);
            m.operand(ctx.imm((u64)ctx.code_sz()));
            auto b = ctx.add(operation::branch).operand(cond);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            m.operand(ctx.imm((u64)ctx.code_sz()));
            ctx.add(operation::jump).operand(ctx.imm(cond_addr));
            b.operand(ctx.imm((u64)ctx.code_sz()));

            ctx.pop_node();
        }
        
        void do_while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);

            auto m = ctx.add(operation::meta_do_while_loop);
            u64 start_addr = ctx.code_sz();

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            var cond = expression(ctx, n->condition);
            m.operand(ctx.imm((u64)ctx.code_sz()));
            auto b = ctx.add(operation::branch).operand(cond);

            ctx.add(operation::jump).operand(ctx.imm(start_addr));

            b.operand(ctx.imm((u64)ctx.code_sz()));

            ctx.pop_node();
        }
    };
};