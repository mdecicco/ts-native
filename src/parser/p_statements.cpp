#include <parser/parse.h>
#include <parser/ast.h>
#include <parser/context.h>
#include <common/errors.h>
#include <common/context.h>
#include <common/module.h>
#include <common/script_type.h>

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

        ast* import_statement(context& ctx) {
            if (!ctx.match({ "import" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current().src, "import");
            }

            ast* stmt = new ast();
            stmt->type = nt::import_statement;
            stmt->src(ctx.current());
            ctx.consume();

            std::vector<std::string> symbols;
            std::vector<ast*> sym_alias;
            std::string from;
            std::string as;

            if (ctx.match({ tt::open_block })) {
                ctx.consume();
                do {
                    if (symbols.size() > 0) {
                        if (!ctx.match({ tt::comma })) throw exc(ec::p_expected_char, ctx.current().src, ',');
                        ctx.consume();
                    }
                    ast* n = identifier(ctx);
                    symbols.push_back(*n);

                    if (ctx.match({ "as" })) {
                        ctx.consume();
                        ast* alias = identifier(ctx);
                        sym_alias.push_back(alias);
                        n->identifier = alias;
                    } else sym_alias.push_back(nullptr);

                    if (!stmt->body) stmt->body = n;
                    else {
                        ast* b = stmt->body;
                        while (b->next) b = b->next;
                        b->next = n;
                    }
                } while(!ctx.at_end() && !ctx.match({ tt::close_block }));

                if (!ctx.match({ tt::close_block })) throw exc(ec::p_unexpected_eof, stmt->ref, "import body");
                ctx.consume();

                if (!ctx.match({ "from" })) throw exc(ec::p_expected_specific_keyword, ctx.current().src, "from");
                ctx.consume();

                if (!ctx.match({ tt::string_literal })) throw exc(ec::p_expected_import_path, ctx.current().src);

                from = ctx.current().text;
                ast* b = new ast();
                b->type = nt::constant;
                b->src(ctx.current());
                ctx.consume();

                b->next = stmt->body;
                stmt->body = b;
            } else if (ctx.match({ tt::string_literal })) {
                from = ctx.current().text;
                ast* b = new ast();
                b->type = nt::constant;
                b->src(ctx.current());
                ctx.consume();

                stmt->body = b;

                if (ctx.match({ "as" })) {
                    ctx.consume();
                    stmt->identifier = identifier(ctx);
                    as = *stmt->identifier;
                }
            } else throw exc(ec::p_malformed_import, stmt->ref);

            // The module must be resolved
            script_module* mod = ctx.env->resolve(stmt->ref.filename, from);
            stmt->body->set(mod->name());

            if (symbols.size() > 0) {
                // import specific symbols to the global scope
                // only types matter in this context
                for (u16 i = 0;i < symbols.size();i++) {
                    script_type* tp = mod->types()->get(hash(symbols[i]));
                    if (tp) {
                        ctx.type_names.push_back(sym_alias[i] ? *sym_alias[i] : symbols[i]);
                    }
                }
            } else if (as.length() > 0) {
                // import all symbols, accessible member-style under specific name
                ctx.named_imports.push_back({ as, {} });
                auto types = mod->types()->all();
                auto& m = ctx.named_imports.back();
                for (u16 i = 0;i < types.size();i++) m.second.push_back(types[i]->name);
            } else {
                // import all symbols to the global scope
                auto types = mod->types()->all();
                for (u16 i = 0;i < types.size();i++) ctx.type_names.push_back(types[i]->name);
            }

            return stmt;
        }
    };
};