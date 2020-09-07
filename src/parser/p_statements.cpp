#include <parser/parse.h>
#include <parser/ast.h>
#include <parser/context.h>
#include <errors.h>

namespace gjs {
    using tt = lex::token_type;
    using exc = error::exception;
    using ec = error::ecode;
    using token = lex::token;

    namespace parse {
        using nt = ast::node_type;

        ast* if_statement(context& ctx) {
            if (!ctx.match({ "if" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "if");
            }

            ast* stmt = new ast();
            stmt->type = nt::if_statement;
            stmt->src(ctx.current());
            ctx.consume();

            ctx.path.push(stmt);

            if (!ctx.match({ tt::open_parenth })) throw exc(ec::p_expected_char, ctx.current().src, '(');
            stmt->condition = expression(ctx);

            stmt->body = any(ctx);

            if (ctx.match({ "else" })) {
                ctx.consume();
                stmt->else_body = any(ctx);
            }

            ctx.path.pop();
            return stmt;
        }

        ast* return_statement(context& ctx) {
            if (!ctx.match({ "return" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "return");
            }
            
            ast* stmt = new ast();
            stmt->type = nt::return_statement;
            stmt->src(ctx.current());
            ctx.consume();

            if (ctx.match({ tt::semicolon })) {
                ctx.consume();
                return stmt;
            }

            stmt->body = expression(ctx);

            return stmt;
        }

        ast* delete_statement(context& ctx) {
            if (!ctx.match({ "delete" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "delete");
            }

            ast* stmt = new ast();
            stmt->type = nt::delete_statement;
            stmt->src(ctx.current());
            ctx.consume();
            stmt->rvalue = expression(ctx);
            return stmt;
        }
    };
};