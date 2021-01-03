#include <gjs/parser/parse.h>
#include <gjs/parser/ast.h>
#include <gjs/parser/context.h>
#include <gjs/common/errors.h>

namespace gjs {
    using tt = lex::token_type;
    using exc = error::exception;
    using ec = error::ecode;
    using token = lex::token;

    namespace parse {
        using nt = ast::node_type;

        ast* while_loop(context& ctx) {
            if (!ctx.match({ "while" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "while");
            }

            ast* r = new ast();
            r->type = nt::while_loop;
            r->src(ctx.current());
            ctx.consume();

            ctx.path.push(r);

            if (!ctx.match({ tt::open_parenth })) throw exc(ec::p_expected_char, ctx.current().src, '(');
            r->condition = expression(ctx);

            r->body = any(ctx);

            ctx.path.pop();

            return r;
        }

        ast* for_loop(context& ctx) {
            if (!ctx.match({ "for" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "for");
            }

            ast* r = new ast();
            r->type = nt::for_loop;
            r->src(ctx.current());
            ctx.consume();

            if (!ctx.match({ tt::open_parenth })) throw exc(ec::p_expected_char, ctx.current().src, '(');
            ctx.consume();

            if (!ctx.match({ tt::semicolon })) {
                // todo: multi-declaration, separated by commas

                if (ctx.match({ tt::identifier }) && ctx.is_typename(ctx.current())) r->initializer = variable_declaration(ctx);
                else r->initializer = expression(ctx);

                if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
                ctx.consume();
            } else ctx.consume();

            if (!ctx.match({ tt::semicolon })) {
                r->condition = expression(ctx);
                if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
                ctx.consume();
            } else ctx.consume();

            if (!ctx.match({ tt::close_parenth })) {
                // todo: multi-modifier expression, separated by commas
                r->modifier = expression(ctx);
                if (!ctx.match({ tt::close_parenth })) throw exc(ec::p_expected_char, ctx.current().src, ')');
                ctx.consume();
            } else ctx.consume();

            r->body = any(ctx);

            return r;
        }

        ast* do_while_loop(context& ctx) {
            if (!ctx.match({ "do" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "do");
            }

            ast* r = new ast();
            r->type = nt::do_while_loop;
            r->src(ctx.current());
            ctx.consume();

            ctx.path.push(r);

            r->body = any(ctx);

            if (!ctx.match({ "while" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "while");
            }
            ctx.consume();

            if (!ctx.match({ tt::open_parenth })) throw exc(ec::p_expected_char, ctx.current().src, '(');
            r->condition = expression(ctx);

            ctx.path.pop();

            return r;
        }
    };
};