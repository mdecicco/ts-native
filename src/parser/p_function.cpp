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

        ast* function_declaration(context& ctx) {
            ast* r = new ast();
            r->type = nt::function_declaration;

            if (ctx.match({ "operator" })) {
                // cast operator
                r->identifier = new ast();
                r->identifier->type = nt::identifier;
                r->identifier->src(ctx.current());
                r->src(ctx.current());
                ctx.consume();
                r->data_type = type_identifier(ctx);
                r->identifier->set("operator " + std::string(*r->data_type));
            } else {
                r->src(ctx.current());
                r->data_type = type_identifier(ctx);

                if (ctx.match({ "operator" })) {
                    // non-cast operator
                    r->identifier = new ast();
                    r->identifier->type = nt::identifier;
                    r->identifier->src(ctx.current());
                    ctx.consume();

                    if (!ctx.match({ tt::operation })) throw exc(ec::p_expected_operator, ctx.current().src);
                    r->identifier->set("operator " + ctx.current().text);
                } else {
                    // regular function or method
                    r->identifier = identifier(ctx);
                }
            }

            r->arguments = argument_list(ctx);

            if (ctx.match({ tt::semicolon })) {
                ctx.consume();
                return r;
            }

            r->body = block(ctx);
            return r;
        }

        ast* argument_list(context& ctx) {
            if (!ctx.match({ tt::open_parenth })) {
                throw exc(ec::p_expected_char, ctx.current().src, '(');
            }

            ast* r = new ast();
            r->type = nt::function_arguments;
            r->src(ctx.current());

            ctx.consume();

            bool expect_end = false;
            while (!ctx.match({ tt::close_parenth })) {
                if (expect_end) throw exc(ec::p_expected_char, ctx.current().src, ')');

                ast* arg = variable_declaration(ctx);
                if (!r->body) r->body = arg;
                else {
                    ast* n = r->body;
                    while(n->next) n = n->next;
                    n->next = arg;
                }

                if (ctx.match({ tt::comma })) {
                    ctx.consume();
                } else expect_end = true;
            }

            ctx.consume(); // closing parenthesis

            return r;
        }
    };
};