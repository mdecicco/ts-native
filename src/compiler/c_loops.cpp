#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/parser/ast.h>

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
            label_id cond_label = ctx.label();
            if (n->condition) {
                var cond = expression(ctx, n->condition);
                meta.label(ctx.label());
                b = ctx.add(operation::branch).operand(cond);
            } else meta.label(0);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            if (n->modifier) {
                var m = expression(ctx, n->modifier);
            }

            meta.label(ctx.label());
            ctx.add(operation::jump).label(cond_label);

            if (b) b.label(ctx.label());

            if (n->initializer && n->initializer->type == parse::ast::node_type::variable_declaration) ctx.pop_block();
            ctx.pop_node();
        }

        void while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            auto m = ctx.add(operation::meta_while_loop);
            label_id cond_label = ctx.label();
            var cond = expression(ctx, n->condition);
            m.label(ctx.label()); // branch label
            auto b = ctx.add(operation::branch).operand(cond);

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            m.label(ctx.label()); // end label
            ctx.add(operation::jump).label(cond_label);
            b.label(ctx.label()); // post-loop label

            ctx.pop_node();
        }
        
        void do_while_loop(context& ctx, parse::ast* n) {
            ctx.push_node(n);

            auto m = ctx.add(operation::meta_do_while_loop);
            label_id start_label = ctx.label();

            ctx.push_block();
            any(ctx, n->body);
            ctx.pop_block();

            var cond = expression(ctx, n->condition);
            m.label(ctx.label()); // branch label
            auto b = ctx.add(operation::branch).operand(cond);

            ctx.add(operation::jump).label(start_label);
            b.label(ctx.label()); // post-loop label

            ctx.pop_node();
        }
    };
};