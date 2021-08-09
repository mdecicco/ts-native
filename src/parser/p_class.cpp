#include <gjs/parser/parse.h>
#include <gjs/parser/context.h>
#include <gjs/parser/ast.h>
#include <gjs/common/errors.h>

namespace gjs {
    using tt = lex::token_type;
    using exc = error::exception;
    using ec = error::ecode;
    using token = lex::token;

    namespace parse {
        using nt = ast::node_type;

        ast* class_declaration(context& ctx) {
            if (!ctx.match({ "class" })) throw exc(ec::p_expected_specific_keyword, ctx.current().src, "class");

            ast* c = new ast();
            c->type = nt::class_declaration;
            c->src(ctx.current());
            ctx.consume();
            c->identifier = identifier(ctx);

            ctx.type_names.push_back(*c->identifier);

            if (ctx.match_s({ "<", "subtype", ">" })) {
                ctx.consume();
                ctx.consume();
                ctx.consume();
                c->is_subtype = true;
            }

            if (ctx.match({ tt::semicolon })) {
                ctx.consume();
                return c;
            }

            if (!ctx.match({ tt::open_block })) throw exc(ec::p_expected_char, ctx.current().src, '{');
            ctx.consume();
            ctx.path.push(c);

            auto add_to_body = [c](ast* node) {
                if (!c->body) c->body = node;
                else {
                    ast* n = c->body;
                    while (n->next) n = n->next;
                    n->next = node;
                }
            };

            while (!ctx.at_end() && !ctx.match({ tt::close_block })) {
                if (ctx.match({ "constructor" })) {
                    ast* f = new ast();
                    f->type = nt::function_declaration;
                    f->identifier = new ast();
                    f->identifier->type = nt::identifier;
                    f->identifier->set("constructor");
                    f->identifier->src(ctx.current());
                    f->data_type = new ast();
                    f->data_type->type = nt::type_identifier;
                    f->data_type->set(std::string(*c->identifier));
                    f->data_type->src(ctx.current());
                    f->src(ctx.current());
                    ctx.consume();

                    ctx.path.push(f);
                    f->arguments = argument_list(ctx);

                    if (ctx.match({ tt::colon })) {
                        ctx.consume();

                        bool expectMore = false;
                        f->initializer = nullptr;
                        ast* lastInitializer = nullptr;
                        while (!ctx.match({ tt::open_block })) {
                            if (lastInitializer) lastInitializer = lastInitializer->next = new ast();
                            else lastInitializer = f->initializer = new ast();

                            ast* i = lastInitializer;
                            i->src(ctx.current());
                            i->type = nt::property_initializer;
                            i->identifier = identifier(ctx);

                            if (!ctx.match({ tt::open_parenth })) {
                                throw exc(ec::p_expected_char, ctx.current().src, '(');
                            }
                            ctx.consume();

                            ast* lastArg = nullptr;
                            while (!ctx.match({ tt::close_parenth })) {
                                if (lastArg) lastArg = lastArg->next = expression(ctx);
                                else lastArg = i->arguments = expression(ctx);

                                if (ctx.match({ tt::comma })) {
                                    ctx.consume();
                                    expectMore = true;
                                }
                                else {
                                    if (!ctx.match({ tt::close_parenth })) {
                                        throw exc(ec::p_expected_char, ctx.current().src, ')');
                                    }
                                    expectMore = false;
                                }
                            }

                            if (expectMore) {
                                throw exc(ec::p_expected_expression, ctx.current().src);
                            }

                            ctx.consume();
                            
                            if (ctx.match({ tt::comma })) {
                                ctx.consume();
                                expectMore = true;
                            }
                            else {
                                if (!ctx.match({ tt::open_block })) {
                                    throw exc(ec::p_expected_char, ctx.current().src, '{');
                                }
                                expectMore = false;
                            }
                        }

                        if (expectMore) {
                            throw exc(ec::p_expected_identifier, ctx.current().src);
                        }
                    }

                    if (!ctx.match({ tt::semicolon })) {
                        f->body = any(ctx);
                    } else ctx.consume();
                    ctx.path.pop();

                    add_to_body(f);
                    continue;
                }

                if (ctx.match({ "destructor" })) {
                    ast* f = new ast();
                    f->type = nt::function_declaration;
                    f->identifier = new ast();
                    f->identifier->type = nt::identifier;
                    f->identifier->set("destructor");
                    f->identifier->src(ctx.current());
                    f->data_type = new ast();
                    f->data_type->type = nt::type_identifier;
                    f->data_type->set(std::string(*c->identifier));
                    f->data_type->src(ctx.current());
                    f->src(ctx.current());
                    ctx.consume();

                    ctx.path.push(f);
                    f->arguments = argument_list(ctx);
                    if (!ctx.match({ tt::semicolon })) {
                        f->body = any(ctx);
                    } else ctx.consume();
                    ctx.path.pop();

                    c->destructor = f;
                    continue;
                }

                if (ctx.match({ "const" })) {
                    ast* decl = new ast();
                    decl->type = nt::class_property;
                    decl->src(ctx.current());
                    decl->is_const = true;
                    ctx.consume();
                    decl->data_type = type_identifier(ctx);
                    decl->identifier = identifier(ctx);

                    if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
                    ctx.consume();
                    add_to_body(decl);
                    continue;
                }
                
                if (ctx.match({ "static" })) {
                    if (ctx.pattern({ tt::any, tt::identifier, tt::identifier, tt::open_parenth })) {
                        ctx.consume();
                        ast* f = function_declaration(ctx);
                        f->is_static = true;
                        add_to_body(f);
                        continue;
                    }

                    ast* decl = new ast();
                    decl->type = nt::class_property;
                    decl->src(ctx.current());
                    decl->is_static = true;
                    ctx.consume();
                    decl->data_type = type_identifier(ctx);
                    decl->identifier = identifier(ctx);

                    if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
                    ctx.consume();
                    add_to_body(decl);
                    continue;
                }

                if (ctx.match({ tt::identifier })) {
                    if (ctx.is_typename(ctx.current().text)) {
                        if (ctx.pattern({ tt::identifier, tt::identifier, tt::open_parenth })) {
                            // like 'void print(...' or 'i32 hello(...'
                            add_to_body(function_declaration(ctx));
                            continue;
                        } else if (ctx.pattern({ tt::identifier, tt::keyword, tt::operation, tt::open_parenth }) && ctx.pattern_s({ "", "operator" })) {
                            // like 'i32 operator += (...' or 'vec3f operator * (...'
                            add_to_body(function_declaration(ctx));
                            continue;
                        } else if (ctx.pattern({ tt::identifier, tt::identifier, tt::semicolon })) {
                            // like 'i32 x;' or 'f32 y;'
                            ast* decl = new ast();
                            decl->type = nt::class_property;
                            decl->src(ctx.current());
                            decl->data_type = type_identifier(ctx);
                            decl->identifier = identifier(ctx);
                            ctx.consume(); // ;
                            add_to_body(decl);
                            continue;
                        } 

                        throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
                    } else {
                        throw exc(ec::p_unexpected_identifier, ctx.current().src, ctx.current().text.c_str());
                    }
                    continue;
                }

                if (ctx.match({ tt::keyword })) {
                    if (ctx.pattern({ tt::keyword, tt::identifier, tt::open_parenth }) && ctx.match({ "operator" })) {
                        if (!ctx.is_typename(ctx.tokens[ctx.cur_token + 1].text)) {
                            throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
                        }

                        // like 'operator i32(...' or 'operator vec2f(...'
                        add_to_body(function_declaration(ctx));
                        continue;
                    }
                    
                    throw exc(ec::p_unexpected_keyword, ctx.current().src, ctx.current().text.c_str());
                }
            }

            if (!ctx.match({ tt::close_block })) {
                throw exc(ec::p_unexpected_eof, c->ref, "class body");
            }
            
            ctx.consume();

            ctx.path.pop();
            return c;
        }

        ast* format_declaration(context& ctx) {
            if (!ctx.match({ "format" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current(), "format");
            }
            
            ast* c = new ast();
            c->type = nt::format_declaration;
            c->src(ctx.current());
            ctx.consume();
            c->identifier = identifier(ctx);

            ctx.type_names.push_back(*c->identifier);

            if (ctx.match_s({ "<", "subtype", ">" })) {
                ctx.consume();
                ctx.consume();
                ctx.consume();
                c->is_subtype = true;
            }
            
            if (!ctx.match({ tt::open_block })) throw exc(ec::p_expected_char, ctx.current(), '{');
            ctx.consume();

            bool expect_end = false;
            while (!ctx.at_end() && !ctx.match({ tt::close_block })) {
                if (expect_end) {
                    throw exc(ec::p_expected_char, ctx.current(), '}');
                }

                ast* p = new ast();
                p->type = nt::format_property;
                p->data_type = type_identifier(ctx);
                p->ref = p->data_type->ref;

                p->identifier = identifier(ctx);

                if (!c->body) c->body = p;
                else {
                    ast* n = c->body;
                    while (n->next) n = n->next;
                    n->next = p;
                }

                if (ctx.match({ tt::comma })) ctx.consume();
                else expect_end = true;
            }

            if (!ctx.match({ tt::close_block })) {
                throw exc(ec::p_unexpected_eof, c->ref, "format body");
            }

            ctx.consume();

            return c;
        };

        ast* enum_declaration(context& ctx) {
            if (!ctx.match({ "enum" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current(), "enum");
            }

            ast* c = new ast();
            c->type = nt::enum_declaration;
            c->src(ctx.current());
            ctx.consume();
            c->identifier = identifier(ctx);

            if (!ctx.match({ tt::open_block })) throw exc(ec::p_expected_char, ctx.current(), '{');
            ctx.consume();

            bool expect_end = false;
            while (!ctx.at_end() && !ctx.match({ tt::close_block }) && !expect_end) {
                ast* v = new ast();
                v->type = nt::enum_value;
                v->src(ctx.current());
                v->identifier = identifier(ctx);

                if (ctx.match({ "=" })) {
                    ctx.consume();
                    v->body = expression(ctx);
                }

                if (ctx.match({ tt::comma })) ctx.consume();
                else expect_end = true;

                if (!c->body) c->body = v;
                else {
                    ast* n = c->body;
                    while (n->next) n = n->next;
                    n->next = v;
                }
            }

            if (!ctx.match({ tt::close_block })) throw exc(ec::p_expected_x, ctx.current(), "'}' or ','");
            ctx.consume();

            return c;
        }
    };
};