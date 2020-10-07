#include <parser/parse.h>
#include <parser/ast.h>
#include <parser/context.h>
#include <common/errors.h>

namespace gjs {
    using tt = lex::token_type;
    using exc = error::exception;
    using ec = error::ecode;
    using token = lex::token;

    namespace parse {
        using nt = ast::node_type;
        using op = ast::operation_type;

        ast* assign(context& ctx);
        ast* conditional(context& ctx);
        ast* logical_or(context& ctx);
        ast* logical_and(context& ctx);
        ast* binary_or(context& ctx);
        ast* binary_xor(context& ctx);
        ast* binary_and(context& ctx);
        ast* equality(context& ctx);
        ast* comp(context& ctx);
        ast* shift(context& ctx);
        ast* add(context& ctx);
        ast* mult(context& ctx);
        ast* unary(context& ctx);
        ast* postfix(context& ctx);
        ast* call(context& ctx);
        ast* member(context& ctx);
        ast* primary(context& ctx);
        ast* parse_identifier(const token& tok);
        ast* parse_type_identifier(context& ctx, const token& tok, bool expected = true);
        ast* parse_boolean(const token& tok);
        ast* parse_number(const token& tok);
        ast* parse_string(const token& tok);
        void set_node_src(ast* node, const token& tok) { node->src(tok); }



        ast* expression(context& ctx) {
            return assign(ctx);
        }

        ast* assign(context& ctx) {
            ast* ret = conditional(ctx);
            token cur = ctx.current();
            while (ctx.match({ "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "&&=", "||=" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "=") op->op = op::eq;
                else if (cur.text == "+=") op->op = op::addEq;
                else if (cur.text == "-=") op->op = op::subEq;
                else if (cur.text == "*=") op->op = op::mulEq;
                else if (cur.text == "/=") op->op = op::divEq;
                else if (cur.text == "%=") op->op = op::modEq;
                else if (cur.text == "<<=") op->op = op::shiftLeftEq;
                else if (cur.text == ">>=") op->op = op::shiftRightEq;
                else if (cur.text == "&=") op->op = op::bandEq;
                else if (cur.text == "^=") op->op = op::bxorEq;
                else if (cur.text == "|=") op->op = op::borEq;
                else if (cur.text == "&&=") op->op = op::landEq;
                else if (cur.text == "||=") op->op = op::lorEq;
                op->lvalue = ret;
                op->rvalue = conditional(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* conditional(context& ctx) {
            ast* ret = logical_or(ctx);

            token cur = ctx.current();
            if (ctx.match({ tt::question_mark })) {
                ctx.consume();
                ast* cond = new ast();
                cond->type = nt::conditional;
                cond->condition = ret;
                cond->lvalue = expression(ctx);
                if (!cond->lvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                if (!ctx.match({ tt::colon })) throw exc(ec::p_expected_char, ctx.current().src, ':');
                ctx.consume();

                cond->rvalue = expression(ctx);
                if (!cond->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                set_node_src(cond, cur);
                ret = cond;
            }

            return ret;
        }

        ast* logical_or(context& ctx) {
            ast* ret = logical_and(ctx);
            token cur = ctx.current();
            while (ctx.match({ "||" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::lor;
                op->lvalue = ret;
                op->rvalue = logical_and(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* logical_and(context& ctx) {
            ast* ret = binary_or(ctx);
            token cur = ctx.current();
            while (ctx.match({ "&&" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::land;
                op->lvalue = ret;
                op->rvalue = binary_or(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* binary_or(context& ctx) {
            ast* ret = binary_xor(ctx);
            token cur = ctx.current();
            while (ctx.match({ "|" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::bor;
                op->lvalue = ret;
                op->rvalue = binary_xor(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* binary_xor(context& ctx) {
            ast* ret = binary_and(ctx);
            token cur = ctx.current();
            while (ctx.match({ "^" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::bxor;
                op->lvalue = ret;
                op->rvalue = binary_and(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* binary_and(context& ctx) {
            ast* ret = equality(ctx);
            token cur = ctx.current();
            while (ctx.match({ "&" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::band;
                op->lvalue = ret;
                op->rvalue = equality(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* equality(context& ctx) {
            ast* ret = comp(ctx);
            token cur = ctx.current();
            while (ctx.match_s({ "!=", "==" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "!=") op->op = op::notEq;
                else op->op = op::isEq;
                op->lvalue = ret;
                op->rvalue = comp(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* comp(context& ctx) {
            ast* ret = shift(ctx);
            token cur = ctx.current();
            while (ctx.match({ "<", "<=", ">", ">=" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "<") op->op = op::less;
                else if (cur.text == "<=") op->op = op::lessEq;
                else if (cur.text == ">") op->op = op::greater;
                else op->op = op::greaterEq;
                op->lvalue = ret;
                op->rvalue = shift(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* shift(context& ctx) {
            ast* ret = add(ctx);
            token cur = ctx.current();
            while (ctx.match_s({ "<<", ">>" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "<<") op->op = op::shiftLeft;
                else op->op = op::shiftRight;
                op->lvalue = ret;
                op->rvalue = add(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* add(context& ctx) {
            ast* ret = mult(ctx);
            token cur = ctx.current();
            while (ctx.match_s({ "+", "-" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "+") op->op = op::add;
                else op->op = op::sub;
                op->lvalue = ret;
                op->rvalue = mult(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* mult(context& ctx) {
            ast* ret = unary(ctx);
            token cur = ctx.current();
            while (ctx.match({ "*", "/", "%" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "*") op->op = op::mul;
                else if (cur.text == "/") op->op = op::div;
                else op->op = op::mod;
                op->lvalue = ret;
                op->rvalue = unary(ctx);
                if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* unary(context& ctx) {
            token cur = ctx.current();
            if (ctx.match({ "!", "-", "--", "++" })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "!") op->op = op::not;
                else if (cur.text == "-") op->op = op::negate;
                else if (cur.text == "--") op->op = op::preDec;
                else if (cur.text == "++") op->op = op::preInc;
                op->rvalue = unary(ctx);
                if (!op->rvalue && (op->op == op::not || op->op == op::negate)) throw exc(ec::p_expected_expression, cur.src);
                if (!op->rvalue || (op->rvalue->type != nt::identifier && op->rvalue->op != op::member && op->rvalue->op != op::index)) {
                    throw exc(ec::p_expected_assignable, op->rvalue ? op->rvalue->ref : ctx.current().src);
                }

                set_node_src(op, cur);
                return op;
            }

            return postfix(ctx);
        }

        ast* postfix(context& ctx) {
            ast* ret = call(ctx);
            token cur = ctx.current();
            if (ctx.match_s({ "++", "--" })) {
                if (!ret || (ret->type != nt::identifier && ret->op != op::member && ret->op != op::index)) {
                    throw exc(ec::p_expected_assignable, ret ? ret->ref : ctx.current().src);
                }

                ctx.consume();
                ast* op = new ast();
                op->type = nt::operation;
                if (cur.text == "--") op->op = op::postDec;
                else op->op = op::postInc;
                op->lvalue = ret;
                set_node_src(op, cur);
                return op;
            }

            return ret;
        }

        ast* call(context& ctx) {
            ast* ret = member(ctx);
            token cur = ctx.current();
            while (ctx.match({ tt::open_parenth })) {
                if (!ret || (ret->type != nt::identifier && ret->op != op::member)) {
                    throw exc(ec::p_expected_callable, ret ? ret->ref : ctx.current().src);
                }
                ctx.consume();

                ast* c = new ast();
                c->type = nt::call;
                c->callee = ret;
                c->arguments = expression(ctx);
                while (ctx.match({ tt::comma })) {
                    if (!c->arguments) throw exc(ec::p_expected_expression, cur.src);

                    token comma = ctx.current();
                    ctx.consume();
                    ast* n = c->arguments;
                    while (n->next) n = n->next;
                    n->next = expression(ctx);
                    if (!n->next) throw exc(ec::p_expected_expression, ctx.current().src);
                }
                if (!ctx.match({ tt::close_parenth })) throw exc(ec::p_expected_char, ctx.current().src, ')');
                ctx.consume();

                ret = c;
                set_node_src(c, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* member(context& ctx) {
            ast* ret = primary(ctx);
            token cur = ctx.current();
            while (ctx.match({ tt::member_accessor, tt::open_bracket })) {
                ctx.consume();

                ast* op = new ast();
                op->type = nt::operation;
                op->op = cur == tt::open_bracket ? op::index : op::member;
                op->lvalue = ret;
                if (cur == tt::open_bracket) {
                    op->rvalue = primary(ctx);
                    if (!op->rvalue) throw exc(ec::p_expected_expression, ctx.current().src);
                } else {
                    if (ctx.match({ tt::identifier })) op->rvalue = identifier(ctx);
                    else throw exc(ec::p_expected_identifier, ctx.current().src);
                }

                if (cur == tt::open_bracket) {
                    if (!ctx.match({ tt::close_bracket })) throw exc(ec::p_expected_char, ctx.current().src, ']');
                    ctx.consume();
                }

                ret = op;
                set_node_src(op, cur);
                cur = ctx.current();
            }

            return ret;
        }

        ast* primary(context& ctx) {
            ast* n = nullptr;

            n = parse_type_identifier(ctx, ctx.current(), false);
            if (n) {
                token typeTok = ctx.current();
                ctx.consume();

                if (ctx.match({ "<" })) {
                    ctx.consume();
                    n->data_type = parse_type_identifier(ctx, ctx.current());
                    ctx.consume();
                    if (!ctx.match({ ">" })) throw exc(ec::p_expected_char, ctx.current().src, '>');
                    ctx.consume();
                }

                // could be referring to a static method or property
                if (ctx.match({ tt::member_accessor })) return n;

                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::stackObj;
                op->data_type = n;

                if (ctx.match({ tt::open_parenth })) {
                    ctx.consume();

                    op->arguments = expression(ctx);

                    while (ctx.match({ tt::comma })) {
                        if (!op->arguments) throw exc(ec::p_expected_expression, ctx.current().src);

                        token comma = ctx.current();
                        ctx.consume();
                        ast* n = op->arguments;
                        while (n->next) n = n->next;
                        n->next = expression(ctx);
                        if (!n->next) throw exc(ec::p_expected_expression, ctx.current().src);
                    }
                    if (!ctx.match({ tt::close_parenth })) throw exc(ec::p_expected_char, ctx.current().src, ')');
                    ctx.consume();
                }

                set_node_src(op, typeTok);
                return op;
            }

            n = parse_identifier(ctx.current());
            if (n) {
                ctx.consume();
                return n;
            }

            n = parse_boolean(ctx.current());
            if (n) {
                ctx.consume();
                return n;
            }

            n = parse_number(ctx.current());
            if (n) {
                ctx.consume();
                return n;
            }

            n = parse_string(ctx.current());
            if (n) {
                ctx.consume();
                return n;
            }

            if (ctx.match({ tt::open_parenth })) {
                ctx.consume();
                n = expression(ctx);
                if(!ctx.match({ tt::close_parenth })) throw exc(ec::p_expected_char, ctx.current().src, ')');
                ctx.consume();
                return n;
            }

            if (ctx.match({ "new" })) {
                token newOp = ctx.current();
                ast* op = new ast();
                op->type = nt::operation;
                op->op = op::newObj;
                ctx.consume();
                op->data_type = parse_type_identifier(ctx, ctx.current());

                if (!op->data_type) throw exc(ec::p_expected_type_identifier, ctx.current().src);
                ctx.consume();

                if (ctx.match({ "<" })) {
                    ctx.consume();
                    op->data_type->data_type = parse_type_identifier(ctx, ctx.current());
                    ctx.consume();
                    if (!ctx.match({ ">" })) throw exc(ec::p_expected_char, ctx.current().src, '>');
                    ctx.consume();
                }

                if (ctx.match({ tt::open_parenth })) {
                    ctx.consume();

                    op->arguments = expression(ctx);

                    while (ctx.match({ tt::comma })) {
                        if (!op->arguments) throw exc(ec::p_expected_expression, ctx.current().src);

                        token comma = ctx.current();
                        ctx.consume();
                        ast* n = op->arguments;
                        while (n->next) n = n->next;
                        n->next = expression(ctx);
                        if (!n->next) throw exc(ec::p_expected_expression, ctx.current().src);
                    }
                    if (!ctx.match({ tt::close_parenth })) throw exc(ec::p_expected_char, ctx.current().src, ')');
                    ctx.consume();
                }

                set_node_src(op, newOp);
                return op;
            }

            return n;
        }

        ast* parse_identifier(const token& tok) {
            if (tok == "this" || tok == tt::identifier) {
                ast* n = new ast();
                n->type = nt::identifier;
                n->set(tok.text);
                set_node_src(n, tok);
                return n;
            }

            return nullptr;
        }

        ast* parse_type_identifier(context& ctx, const token& tok, bool expected) {
            if (tok == tt::identifier) {
                if (ctx.is_typename(tok.text)) {
                    ast* n = new ast();
                    n->type = nt::type_identifier;
                    n->set(tok.text);
                    set_node_src(n, tok);
                    return n;
                }
            }

            if (expected) throw exc(ec::p_expected_type_identifier, tok.src);
            return nullptr;
        }

        ast* parse_boolean(const token& tok) {
            if (tok == "true" || tok == "false") {
                ast* node = new ast();
                node->type = nt::constant;
                node->c_type = ast::constant_type::integer;
                node->value.i = tok.text == "true" ? 1 : 0;
                set_node_src(node, tok);
                return node;
            }

            return nullptr;
        }

        ast* parse_number(const token& tok) {
            if (tok.text == "null") {
                ast* node = new ast();
                node->type = nt::constant;
                node->c_type = ast::constant_type::integer;
                node->value.i = 0;
                set_node_src(node, tok);
                return node;
            }

            if (tok == tt::integer_literal) {
                ast* node = new ast();
                node->type = nt::constant;
                node->c_type = ast::constant_type::integer;
                node->value.i =    atoi(tok.text.c_str());
                set_node_src(node, tok);
                return node;
            }
            
            if (tok == tt::decimal_literal) {
                ast* node = new ast();
                node->type = nt::constant;
                if (tok.text[tok.text.length() - 1] == 'f') {
                    node->c_type = ast::constant_type::f32;
                    node->value.f_32 = (f32)atof(tok.text.c_str());
                } else {
                    node->c_type = ast::constant_type::f64;
                    node->value.f_64 = atof(tok.text.c_str());
                }
                set_node_src(node, tok);
                return node;
            }

            return nullptr;
        }

        ast* parse_string(const token& tok) {
            if (tok == tt::string_literal) {
                ast* node = new ast();
                node->type = nt::constant;
                node->c_type = ast::constant_type::string;
                node->set(tok.text);
                set_node_src(node, tok);
                return node;
            }
            return nullptr;
        }
    };
};