#include <gs/compiler/Parser.h>
#include <gs/compiler/Lexer.h>
#include <gs/utils/ProgramSource.h>
#include <utils/Array.hpp>

#include <stdarg.h>
#include <iostream>

namespace gs {
    namespace compiler {
        //
        // ast_node
        //
        
        utils::String ast_node::str() const {
            if (str_len == 0) return utils::String();
            return utils::String(const_cast<char*>(value.s), str_len);
        }
        
        void iprintf(u32 indent, const char* fmt, ...) {
            va_list l;
            va_start(l, fmt);
            for (u32 i = 0;i < indent;i++) printf("    ");
            vprintf(fmt, l);
            va_end(l);    
        }
        
        void getNodeEndLoc(const ast_node* n, SourceLocation* out, bool ignoreNext = true) {
            if (!n) return;

            if (n->tok.src.getLine() > out->getLine()) {
                *out = SourceLocation(
                    n->tok.src.getSource(),
                    n->tok.src.getLine(),
                    n->tok.src.getCol() + n->tok.text.size()
                );
            } else if (n->tok.src.getLine() == out->getLine() && n->tok.src.getCol() + n->tok.text.size() > out->getCol()) {
                *out = SourceLocation(
                    n->tok.src.getSource(),
                    n->tok.src.getLine(),
                    n->tok.src.getCol() + n->tok.text.size()
                );
            }
            
            getNodeEndLoc(n->data_type, out, false);
            getNodeEndLoc(n->lvalue, out, false);
            getNodeEndLoc(n->rvalue, out, false);
            getNodeEndLoc(n->cond, out, false);
            getNodeEndLoc(n->body, out, false);
            getNodeEndLoc(n->else_body, out, false);
            getNodeEndLoc(n->initializer, out, false);
            getNodeEndLoc(n->parameters, out, false);
            getNodeEndLoc(n->template_parameters, out, false);
            getNodeEndLoc(n->modifier, out, false);
            getNodeEndLoc(n->alias, out, false);
            getNodeEndLoc(n->inheritance, out, false);


            if (!ignoreNext) getNodeEndLoc(n->next, out, false);
        }

        void ast_node::computeSourceLocationRange() {
            if (tok.src.m_length > 0) return;

            SourceLocation end = tok.src;
            getNodeEndLoc(this, &end);
            
            tok.src.m_length = end.getOffset() - tok.src.getOffset();
            tok.src.m_endLine = end.m_line;
            tok.src.m_endCol = end.m_col;
        }
        
        void ast_node::manuallySpecifyRange(const token& end) {
            tok.src.m_length = end.src.getOffset() - tok.src.getOffset();
            tok.src.m_endLine = end.src.m_line;
            tok.src.m_endCol = end.src.m_col;
        }

        void ast_node::json(u32 indent, u32 index, bool noIndentOpenBrace) {
            static const char* nts[] = {
                "empty",
                "error",
                "root",
                "eos",
                "break",
                "cast",
                "catch",
                "class",
                "continue",
                "export",
                "expression",
                "function_type",
                "function",
                "identifier",
                "if",
                "import_module",
                "import_symbol",
                "import",
                "literal",
                "loop",
                "object_decompositor",
                "object_literal_property",
                "parameter",
                "property",
                "return",
                "scoped_block",
                "sizeof",
                "switch_case",
                "switch",
                "this",
                "throw",
                "try",
                "type_modifier",
                "type_property",
                "type_specifier",
                "type",
                "variable"
            };
            static const char* ops[] = {
                "undefined",
                "add",
                "addEq",
                "sub",
                "subEq",
                "mul",
                "mulEq",
                "div",
                "divEq",
                "mod",
                "modEq",
                "xor",
                "xorEq",
                "bitAnd",
                "bitAndEq",
                "bitOr",
                "bitOrEq",
                "bitInv",
                "shLeft",
                "shLeftEq",
                "shRight",
                "shRightEq",
                "not",
                "notEq",
                "logAnd",
                "logAndEq",
                "logOr",
                "logOrEq",
                "assign",
                "compare",
                "lessThan",
                "lessThanEq",
                "greaterThan",
                "greaterThanEq",
                "preInc",
                "postInc",
                "preDec",
                "postDec",
                "negate",
                "index",
                "conditional",
                "member",
                "new",
                "placementNew",
                "call"
            };
            static const char* lts[] = {
                "u8",
                "u16",
                "u32",
                "u64",
                "i8",
                "i16",
                "i32",
                "i64",
                "f32",
                "f64",
                "string",
                "template_string",
                "object",
                "array",
                "null",
                "true",
                "false"
            };

            if (next && index == 0) {
                printf("[\n");
                indent++;
                noIndentOpenBrace = false;
            }

            if (noIndentOpenBrace) printf("{\n");
            else iprintf(indent, "{\n");

            indent++;
            iprintf(indent, "\"type\": \"%s\",\n", nts[tp]);

            if (tok.src.isValid()) {
                computeSourceLocationRange();

                utils::String ln = tok.src.getSource()->getLine(tok.src.getLine()).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");
                u32 wsc = 0;
                for (;wsc < ln.size();wsc++) {
                    if (isspace(ln[wsc])) continue;
                    break;
                }

                SourceLocation end = tok.src.getEndLocation();
                u32 baseOffset = tok.src.getOffset();
                u32 endOffset = end.getOffset();

                utils::String indxStr = "";
                for (u32 i = 0;i < (tok.src.getCol() - wsc);i++) indxStr += ' ';
                for (u32 i = 0;i < (endOffset - baseOffset) && indxStr.size() < (ln.size() - wsc);i++) {
                    indxStr += '^';
                }


                iprintf(indent, "\"source\": {\n");
                iprintf(indent + 1, "\"range\": {\n");
                iprintf(indent + 1, "    \"start\": {\n");
                iprintf(indent + 1, "        \"line\": %d,\n", tok.src.getLine());
                iprintf(indent + 1, "        \"col\": %d,\n", tok.src.getCol());
                iprintf(indent + 1, "        \"offset\": %d\n", baseOffset);
                iprintf(indent + 1, "    },\n");
                iprintf(indent + 1, "    \"end\": {\n");
                iprintf(indent + 1, "        \"line\": %d,\n", end.getLine());
                iprintf(indent + 1, "        \"col\": %d,\n", end.getCol());
                iprintf(indent + 1, "        \"offset\": %d\n", endOffset);
                iprintf(indent + 1, "    }\n");
                iprintf(indent + 1, "},\n");
                iprintf(indent + 1, "\"line_text\": \"%s\",\n", ln.c_str() + wsc);
                iprintf(indent + 1, "\"indx_text\": \"%s\"\n", indxStr.c_str());
                iprintf(indent, "}");
            } else {
                iprintf(indent, "\"source\": null");
            }
            
            if (tp == nt_literal) {
                printf(",\n");
                iprintf(indent, "\"value_type\": \"%s\"", lts[value_tp]);

                utils::String val;
                switch (value_tp) {
                    case lt_u8:
                    case lt_u16:
                    case lt_u32:
                    case lt_u64: {
                        printf(",\n");
                        iprintf(indent, "\"value\": %llu", value.u);
                        break;
                    }
                    case lt_i8:
                    case lt_i16:
                    case lt_i32:
                    case lt_i64: {
                        printf(",\n");
                        iprintf(indent, "\"value\": %lli", value.i);
                        break;
                    }
                    case lt_f32:
                    case lt_f64: {
                        printf(",\n");
                        iprintf(indent, "\"value\": %f", value.f);
                        break;
                    }
                    case lt_string:
                    case lt_template_string: {
                        printf(",\n");
                        iprintf(indent, "\"value\": \"%s\"", str().c_str());
                        break;
                    }
                    case lt_object:
                    case lt_array: {
                        printf(",\n");
                        iprintf(indent, "\"value\": ");
                        body->json(indent + 1);
                        break;
                    }
                    case lt_null: {
                        printf(",\n");
                        iprintf(indent, "\"value\": null");
                        break;
                    }
                    case lt_true: {
                        printf(",\n");
                        iprintf(indent, "\"value\": true");
                        break;
                    }
                    case lt_false: {
                        printf(",\n");
                        iprintf(indent, "\"value\": false");
                        break;
                    }
                }
            } else if (str_len > 0) {
                printf(",\n");
                iprintf(indent, "\"text\": \"%s\"", str().c_str());
            }
            
            if (op != op_undefined) {
                printf(",\n");
                iprintf(indent, "\"operation\": \"%s\"", ops[op]);
            }

            printf(",\n");
            iprintf(indent, "\"flags\": [");
            u32 i = 0;
            if (flags.is_const) printf("%s\"is_const\"", (i++) > 0 ? ", " : "");
            if (flags.is_static) printf("%s\"is_static\"", (i++) > 0 ? ", " : "");
            if (flags.is_private) printf("%s\"is_private\"", (i++) > 0 ? ", " : "");
            if (flags.is_array) printf("%s\"is_array\"", (i++) > 0 ? ", " : "");
            if (flags.is_pointer) printf("%s\"is_pointer\"", (i++) > 0 ? ", " : "");
            if (flags.defer_cond) printf("%s\"defer_cond\"", (i++) > 0 ? ", " : "");
            printf("]");

            if (data_type) {
                printf(",\n");
                iprintf(indent, "\"data_type\": ");
                data_type->json(indent);
            }
            if (lvalue) {
                printf(",\n");
                iprintf(indent, "\"lvalue\": ");
                lvalue->json(indent);
            }
            if (rvalue) {
                printf(",\n");
                iprintf(indent, "\"rvalue\": ");
                rvalue->json(indent);
            }
            if (cond) {
                printf(",\n");
                iprintf(indent, "\"cond\": ");
                cond->json(indent);
            }
            if (body) {
                printf(",\n");
                iprintf(indent, "\"body\": ");
                body->json(indent);
            }
            if (else_body) {
                printf(",\n");
                iprintf(indent, "\"else_body\": ");
                else_body->json(indent);
            }
            if (initializer) {
                printf(",\n");
                iprintf(indent, "\"initializer\": ");
                initializer->json(indent);
            }
            if (parameters) {
                printf(",\n");
                iprintf(indent, "\"parameters\": ");
                parameters->json(indent);
            }
            if (template_parameters) {
                printf(",\n");
                iprintf(indent, "\"template_parameters\": ");
                template_parameters->json(indent);
            }
            if (modifier) {
                printf(",\n");
                iprintf(indent, "\"modifier\": ");
                modifier->json(indent);
            }
            if (alias) {
                printf(",\n");
                iprintf(indent, "\"alias\": ");
                alias->json(indent);
            }
            if (inheritance) {
                printf(",\n");
                iprintf(indent, "\"extends\": ");
                inheritance->json(indent);
            }

            printf("\n");
            iprintf(indent > 0 ? indent - 1 : 0, "}");

            if (next) {
                indent--;
                printf(",\n");
                next->json(indent, index + 1, false);

                if (index == 0) {
                    printf("\n");
                    iprintf(--indent, "]");
                }
            }
        }
        
        ast_node* ast_node::clone(bool copyNext) {
            ast_node* c = new ast_node();
            c->tok = tok;
            c->tp = tp;
            c->value_tp = value_tp;
            c->op = op;
            c->str_len = str_len;
            c->value = value;
            c->flags = flags;

            if (str_len > 0) {
                c->value.s = new char[str_len + 1];
                for (u32 i = 0;i < str_len;i++) {
                    const_cast<char*>(c->value.s)[i] = value.s[i];
                }
                const_cast<char*>(c->value.s)[str_len] = 0;
            }
            
            if (data_type) c->data_type = data_type->clone(true);
            else c->data_type = nullptr;

            if (lvalue) c->lvalue = lvalue->clone(true);
            else c->lvalue = nullptr;

            if (rvalue) c->rvalue = rvalue->clone(true);
            else c->rvalue = nullptr;

            if (cond) c->cond = cond->clone(true);
            else c->cond = nullptr;

            if (body) c->body = body->clone(true);
            else c->body = nullptr;

            if (else_body) c->else_body = else_body->clone(true);
            else c->else_body = nullptr;

            if (initializer) c->initializer = initializer->clone(true);
            else c->initializer = nullptr;

            if (parameters) c->parameters = parameters->clone(true);
            else c->parameters = nullptr;

            if (template_parameters) c->template_parameters = template_parameters->clone(true);
            else c->template_parameters = nullptr;

            if (modifier) c->modifier = modifier->clone(true);
            else c->modifier = nullptr;

            if (alias) c->alias = alias->clone(true);
            else c->alias = nullptr;

            if (inheritance) c->inheritance = inheritance->clone(true);
            else c->inheritance = nullptr;

            if (copyNext) {
                if (next) c->next = next->clone(true);
                else c->next = nullptr;
            } else c->next = nullptr;

            return c;
        }
        
        void ast_node::destroyDetachedAST(ast_node* n) {
            if (n->str_len > 0) delete [] n->value.s;
            if (n->data_type) ast_node::destroyDetachedAST(n->data_type);
            if (n->lvalue) ast_node::destroyDetachedAST(n->lvalue);
            if (n->rvalue) ast_node::destroyDetachedAST(n->rvalue);
            if (n->cond) ast_node::destroyDetachedAST(n->cond);
            if (n->body) ast_node::destroyDetachedAST(n->body);
            if (n->else_body) ast_node::destroyDetachedAST(n->else_body);
            if (n->initializer) ast_node::destroyDetachedAST(n->initializer);
            if (n->parameters) ast_node::destroyDetachedAST(n->parameters);
            if (n->template_parameters) ast_node::destroyDetachedAST(n->template_parameters);
            if (n->modifier) ast_node::destroyDetachedAST(n->modifier);
            if (n->alias) ast_node::destroyDetachedAST(n->alias);
            if (n->inheritance) ast_node::destroyDetachedAST(n->inheritance);
            if (n->next) ast_node::destroyDetachedAST(n->next);
        }
        
        
        
        //
        // Parser
        //
        
        Parser::Parser(Lexer* l) : m_tokens(l->tokenize()), m_currentIdx(32), m_nodeAlloc(1024, 1024, true) {
            m_currentIdx.push(0);
        }
        
        Parser::~Parser() {
        }
        
        void Parser::push() {
            m_currentIdx.push(m_currentIdx[m_currentIdx.size() - 1]);
            m_currentErrorCount.push(m_errors.size());
        }
        
        void Parser::revert() {
            m_currentIdx.pop();
            m_currentErrorCount.pop();

            if (m_currentErrorCount.size() > 0 && m_currentErrorCount.last() < m_errors.size()) {
                m_errors.remove(m_currentErrorCount.last(), m_errors.size() - m_currentErrorCount.last());
            }
        }
        
        void Parser::commit() {
            u32 idx = m_currentIdx.pop();
            m_currentErrorCount.pop();
            m_currentErrorCount.last() = m_errors.size();
            m_currentIdx[m_currentIdx.size() - 1] = idx;
        }
        
        void Parser::consume() {
            m_currentIdx[m_currentIdx.size() - 1]++;
        }
        
        const token& Parser::get() const {
            return m_tokens[m_currentIdx[m_currentIdx.size() - 1]];
        }
        
        const token& Parser::getPrev() const {
            u32 idx = m_currentIdx[m_currentIdx.size() - 1];
            return m_tokens[idx > 0 ? idx - 1 : 0];
        }
        
        ast_node* Parser::parse() {
            m_nodeAlloc.reset();
            return program(this);
        }
        
        ast_node* Parser::newNode(node_type tp, const token* src) {
            ast_node* n = m_nodeAlloc.alloc();
            n->tok = src ? *src : get();
            n->tp = tp;
            return n;
        }
        
        void Parser::freeNode(ast_node* n) {
            if (n->data_type) freeNode(n->data_type);
            if (n->lvalue) freeNode(n->lvalue);
            if (n->rvalue) freeNode(n->rvalue);
            if (n->cond) freeNode(n->cond);
            if (n->body) freeNode(n->body);
            if (n->else_body) freeNode(n->else_body);
            if (n->initializer) freeNode(n->initializer);
            if (n->template_parameters) freeNode(n->template_parameters);
            if (n->parameters) freeNode(n->parameters);
            if (n->modifier) freeNode(n->modifier);
            if (n->alias) freeNode(n->alias);
            if (n->inheritance) freeNode(n->inheritance);
            if (n->next) freeNode(n->next);
            
            m_nodeAlloc.free(n);
        }
        
        const utils::Array<parse_error>& Parser::errors() const {
            return m_errors;
        }
        
        bool Parser::typeIs(token_type tp) const {
            return get().tp == tp;
        }
        
        bool Parser::textIs(const char* str) const {
            return get().text == str;
        }
        
        bool Parser::isKeyword(const char* str) const {
            const token& t = get();
            return t.tp == tt_keyword && t.text == str;
        }
        
        bool Parser::isSymbol(const char* str) const {
            const token& t = get();
            return t.tp == tt_symbol && t.text == str;
        }
        
        void Parser::error(parse_error_code code, const utils::String& msg) {
            error(code, msg, get());
        }
        
        void Parser::error(parse_error_code code, const utils::String& msg, const token& tok) {
            m_errors.push({
                code,
                msg,
                tok
            });
        }
        
        
        
        //
        // Helpers
        //
        
        ast_node* array_of(Parser* ps, parsefn fn) {
            ast_node* f = fn(ps);
            ast_node* n = f;
            while (n) {
                n->next = fn(ps);
                n = n->next;
            }
            return f;
        }
        ast_node* list_of(
            Parser* ps, parsefn fn,
            parse_error_code before_comma_err, const utils::String& before_comma_msg,
            parse_error_code after_comma_err, const utils::String& after_comma_msg
        ) {
            ast_node* f = fn(ps);
            if (!f) {
                if (before_comma_err != pec_none) {
                    ps->error(before_comma_err, before_comma_msg);
                }
                return nullptr;
            }

            ast_node* n = f;
            while (ps->typeIs(tt_comma)) {
                ps->consume();
                n->next = fn(ps);
                if (!n->next) {
                    ps->error(after_comma_err, after_comma_msg);
                    ps->freeNode(f);
                    return nullptr;
                }

                n = n->next;
            }

            return f;
        }
        ast_node* one_of(Parser* ps, std::initializer_list<parsefn> rules) {
            for (auto fn : rules) {
                ast_node* n = fn(ps);
                if (n) return n;
            }

            return nullptr;
        }
        ast_node* all_of(Parser* ps, std::initializer_list<parsefn> rules) {
            ast_node* f = nullptr;
            ast_node* n = nullptr;
            for (auto fn : rules) {
                if (!f) f = n = fn(ps);
                else {
                    n->next = fn(ps);
                    n = n->next;
                }

                if (!n) {
                    if (f) ps->freeNode(f);
                    return nullptr;
                }
            }

            return f;
        }
        bool isAssignmentOperator(Parser* ps) {
            const token& op = ps->get();
            if (op.tp != tt_symbol) return false;
            // =, +=, -=, *=, /=, %=, <<=, >>=, &=, |=, &&=, ||=, ^=

            if (op.text[0] == '=' && op.text.size() == 1) return true;

            if (op.text.size() == 2 && op.text[1] == '=') {
                // +=, -=, *=, /=, %=, &=, |=, ^= (and >=, <=, ==, !=)
                if (op.text[0] == '>' || op.text[0] == '<' || op.text[0] == '=' || op.text[0] == '!') {
                    // >=, <=, ==, !=
                    return false;
                }

                // Can only be an assignment operator
                // +=, -=, *=, /=, %=, &=, |=, ^=
                return true;
            }

            if (op.text.size() == 3) {
                // The only length-3 symbols are assignment operators
                // &&=, ||=, <<=, >>=
                return true;
            }

            return false;
        }
        expr_operator getOperatorType(Parser* ps) {
            // Note:
            // This function does not cover the following operations
            // - op_placementNew (not overridable)
            // - op_new          (not overridable)
            // - op_member       (not overridable)
            // - op_negate
            //
            // The only one of these that needs special handling due to this is op_negate.
            // When declaring an operator override for a class, "operator -()" will be parsed
            // as if it's an override for the binary subtraction operator. The compiler will
            // interpret an override of the binary subtraction operator as a negation operator
            // override if it takes no arguments.

            const token& op = ps->get();
            if (op.tp != tt_symbol) {
                if (op.tp == tt_open_parenth) {
                    ps->push();
                    ps->consume();
                    if (ps->typeIs(tt_close_parenth)) {
                        ps->revert();
                        return op_call;
                    }
                }

                if (op.tp == tt_open_bracket) {
                    ps->push();
                    ps->consume();
                    if (ps->typeIs(tt_close_bracket)) {
                        ps->revert();
                        return op_index;
                    }
                }

                return op_undefined;
            }

            u32 len = op.text.size();

            switch (op.text[0]) {
                case '+': {
                    if (len == 1) return op_add;
                    if (op.text[1] == '+') return op_undefined; // pre/post unknown
                    return op_addEq;
                }
                case '-': {
                    if (len == 1) return op_sub;
                    if (op.text[1] == '-') return op_undefined; // pre/post unknown
                    return op_subEq;
                }
                case '*': {
                    if (len == 1) return op_mul;
                    return op_mulEq;
                }
                case '/': {
                    if (len == 1) return op_div;
                    return op_divEq;
                }
                case '%': {
                    if (len == 1) return op_mod;
                    return op_modEq;
                }
                case '~': {
                    return op_bitInv;
                }
                case '!': {
                    if (len == 1) return op_not;
                    return op_notEq;
                }
                case '^': {
                    if (len == 1) return op_xor;
                    return op_xorEq;
                }
                case '<': {
                    if (len == 1) return op_lessThan;
                    if (op.text[1] == '<') {
                        if (len == 2) return op_shLeft;
                        return op_shLeftEq;
                    }
                    return op_lessThanEq;
                }
                case '>': {
                    if (len == 1) return op_greaterThan;
                    if (op.text[1] == '>') {
                        if (len == 2) return op_shRight;
                        return op_shRightEq;
                    }
                    return op_greaterThanEq;
                }
                case '=': {
                    if (len == 1) return op_assign;
                    return op_compare;
                }
                case '&': {
                    if (len == 1) return op_bitAnd;
                    if (op.text[1] == '&') {
                        if (len == 2) return op_logAnd;
                        return op_logAndEq;
                    }
                    return op_bitAndEq;
                }
                case '|': {
                    if (len == 1) return op_bitOr;
                    if (op.text[1] == '|') {
                        if (len == 2) return op_logOr;
                        return op_logOrEq;
                    }
                    return op_bitOrEq;
                }
                case '?': return op_conditional;
            }

            return op_undefined;
        }
        
        
        
        //
        // Misc
        //
        
        ast_node* eos(Parser* ps) {
            if (!ps->typeIs(tt_semicolon)) return nullptr;
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ast_node* eosRequired(Parser* ps) {
            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';'");
                return nullptr;
            }
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ast_node* identifier(Parser* ps) {
            if (!ps->typeIs(tt_identifier)) return nullptr;
            ast_node* n = ps->newNode(nt_identifier);
            const token& t = ps->get();
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            ps->consume();
            return n;
        }
        ast_node* typedIdentifier(Parser* ps) {
            ps->push();
            ast_node* id = identifier(ps);
            if (!id) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->freeNode(id);
                ps->revert();
                return nullptr;
            }
            ps->consume();

            id->data_type = typeSpecifier(ps);
            if (!id->data_type) {
                ps->error(pec_expected_type_specifier, "Expected type specifier after ':'");
                // attempt to continue
            }

            ps->commit();
            return id;
        }
        ast_node* objectDecompositorProperty(Parser* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ast_node* objectDecompositor(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->push();
            ps->consume();

            ast_node* f = objectDecompositorProperty(ps);
            if (!f) {
                ps->revert();
                return nullptr;
            }
            ps->commit();

            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = objectDecompositorProperty(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected property after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_brace, "Expected '}' to close object decompositor");
                ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            n = ps->newNode(nt_object_decompositor, tok);
            n->body = f;

            return n;
        }
        
        
        
        //
        // Parameters / Arguments
        //
        
        ast_node* templateArgs(Parser* ps) {
            if (!ps->isSymbol("<")) return nullptr;
            ps->consume();

            ast_node* args = typeSpecifier(ps);
            if (!args) {
                ps->error(pec_expected_type_specifier, "Expected template argument");
                // attempt to continue
            }

            ast_node* n = args;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typeSpecifier(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_type_specifier, "Expected template argument");
                    ps->freeNode(args);
                    return nullptr;
                }
            }

            if (!ps->isSymbol(">")) {
                ps->error(pec_expected_symbol, "Expected '>' after template argument list");
                if (args) ps->freeNode(args);
                return nullptr;
            }

            ps->consume();

            return args;
        }
        ast_node* templateParams(Parser* ps) {
            if (!ps->isSymbol("<")) return nullptr;
            ps->consume();

            ast_node* args = identifier(ps);
            if (!args) {
                ps->error(pec_expected_type_specifier, "Expected template parameter");
                // attempt to continue
            }

            ast_node* n = args;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = identifier(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_type_specifier, "Expected template parameter");
                    ps->freeNode(args);
                    return nullptr;
                }
            }

            if (!ps->isSymbol(">")) {
                ps->error(pec_expected_symbol, "Expected '>' after template parameter list");
                if (args) ps->freeNode(args);
                return nullptr;
            }

            ps->consume();

            return args;
        }
        ast_node* parameter(Parser* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ast_node* typedParameter(Parser* ps) {
            return typedAssignable(ps);
        }
        ast_node* maybeTypedParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->consume();

            ast_node* f = typedParameter(ps);
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typedParameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected typed parameter after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                if (f) ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            return f;
        }
        ast_node* maybeParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->consume();

            ast_node* f = parameter(ps);
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = parameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected parameter after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                if (f) ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            return f;
        }
        ast_node* parameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->consume();

            ast_node* f = parameter(ps);
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = parameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected parameter after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')' to close parameter list");
                if (f) ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            return f;
        }
        ast_node* typedParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->consume();

            ast_node* f = typedParameter(ps);
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typedParameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected parameter after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')' to close parameter list");
                if (f) ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            return f;
        }
        ast_node* arguments(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->consume();

            ast_node* n = expressionSequence(ps);

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')' to close argument list");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            return n;
        }
        
        
        //
        // Types
        //
        
        ast_node* typeModifier(Parser* ps) {
            ast_node* mod = nullptr;
            if (ps->isSymbol("*")) {
                ps->consume();

                mod = ps->newNode(nt_type_modifier, &ps->getPrev());
                mod->flags.is_pointer = 1;
            } else if (ps->typeIs(tt_open_bracket)) {
                const token* t = &ps->get();
                ps->consume();
                if (!ps->typeIs(tt_close_bracket)) {
                    ps->error(pec_expected_closing_bracket, "Expected ']'");
                    // attempt to continue
                } else ps->consume();

                mod = ps->newNode(nt_type_modifier, t);
                mod->flags.is_array = 1;
            }

            if (mod) mod->modifier = typeModifier(ps);
            return mod;
        }
        ast_node* typeProperty(Parser* ps) {
            ps->push();
            ast_node* n = identifier(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            ps->commit();

            n->tp = nt_type_property;
            n->data_type = typeSpecifier(ps);
            if (!n->data_type) {
                ps->error(pec_expected_type_specifier, "Expected type specifier after ':'");
                ps->freeNode(n);
                return nullptr;
            }

            return n;
        }
        ast_node* parenthesizedTypeSpecifier(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token* t = &ps->get();
            ps->push();
            ps->consume();

            ast_node* n = typeSpecifier(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            n->tok = *t;

            ps->commit();

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')'");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            return n;
        }
        ast_node* identifierTypeSpecifier(Parser* ps, ast_node* id) {
            id->template_parameters = templateArgs(ps);
            id->modifier = typeModifier(ps);

            ast_node* n = ps->newNode(nt_type_specifier, &id->tok);
            n->body = id;

            return n;
        }
        ast_node* typeSpecifier(Parser* ps) {
            const token* tok = &ps->get();
            ast_node* id = identifier(ps);
            if (id) return identifierTypeSpecifier(ps, id);

            if (ps->typeIs(tt_open_brace)) {
                ps->push();
                ps->consume();

                ast_node* f = typeProperty(ps);
                if (!f) {
                    ps->revert();
                    return nullptr;
                }
                ps->commit();
                
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after type property");
                    // attempt to continue
                } else ps->consume();

                ast_node* n = f;
                do {
                    n->next = typeProperty(ps);
                    n = n->next;
                    
                    if (!n) break;

                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pec_expected_eos, "Expected ';' after type property");
                        // attempt to continue
                    } else ps->consume();
                } while (true);

                if (!ps->typeIs(tt_close_brace)) {
                    ps->error(pec_expected_closing_brace, "Expected '}' to close object type definition");
                    ps->freeNode(f);
                    return nullptr;
                }
                ps->consume();

                ast_node* t = ps->newNode(nt_type_specifier, tok);
                t->body = f;
                t->modifier = typeModifier(ps);

                return t;
            }

            if (ps->typeIs(tt_open_parenth)) {
                const token* t = &ps->get();
                ps->push();
                ast_node* p = maybeTypedParameterList(ps);
                if (p) {
                    ast_node* n = ps->newNode(nt_type_specifier, t);
                    n->parameters = p;

                    if (ps->typeIs(tt_forward)) {
                        ps->consume();

                        n->data_type = typeSpecifier(ps);
                        if (!n->data_type) {
                            ps->error(pec_expected_type_specifier, "Expected return type specifier");
                            ps->freeNode(n);
                            return nullptr;
                        }

                        return n;
                    } else ps->freeNode(n);
                }

                ps->revert();
            }

            ast_node* n = parenthesizedTypeSpecifier(ps);
            if (n) n->modifier = typeModifier(ps);

            return n;
        }
        ast_node* assignable(Parser* ps) {
            return identifier(ps);
        }
        ast_node* typedAssignable(Parser* ps) {
            ps->push();
            ast_node* id = identifier(ps);
            if (!id) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->freeNode(id);
                ps->revert();
                return nullptr;
            }
            ps->consume();

            id->data_type = typeSpecifier(ps);
            if (!id->data_type) {
                ps->error(pec_expected_type_specifier, "Expected type specifier after ':'");
                // attempt to continue
            }

            ps->commit();
            return id;
        }
        
        
        
        //
        // Literals
        //
        
        ast_node* numberLiteral(Parser* ps) {
            if (!ps->typeIs(tt_number)) return nullptr;

            const token& t = ps->get();
            ps->consume();

            ast_node* n = ps->newNode(nt_literal, &t);

            if (ps->typeIs(tt_number_suffix)) {
                const token& s = ps->get();
                ps->consume();
                char c = s.text[0];
                
                if (c == 'f') n->value_tp = lt_f32;
                else if (c == 'b') n->value_tp = lt_i8;
                else if (c == 's') n->value_tp = lt_i16;
                // if s.text[0] == 'l' then s.text must == 'll'. 'l' is not
                // a valid suffix, it is the default for number literals
                // without decimals
                else if (c == 'l') n->value_tp = lt_i64;
                else if (c == 'u') {
                    c = s.text[1];
                    if (c == 'b') n->value_tp = lt_u8;
                    else if (c == 's') n->value_tp = lt_u16;
                    else if (c == 'l') {
                        if (s.text.size() == 2) n->value_tp = lt_u32;
                        else n->value_tp = lt_u64; // only other option is 'ull'
                    }
                }
            } else {
                if (t.text.firstIndexOf('.') > 0) n->value_tp = lt_f64;
                else n->value_tp = lt_i32;
            }

            switch (n->value_tp) {
                case lt_u8:
                case lt_u16:
                case lt_u32:
                case lt_u64: {
                    n->value.u = std::stoull(t.text.c_str());
                    break;
                }
                case lt_i8:
                case lt_i16:
                case lt_i32:
                case lt_i64: {
                    n->value.i = std::stoll(t.text.c_str());
                    break;
                }
                case lt_f32:
                case lt_f64: {
                    n->value.f = std::stod(t.text.c_str());
                    break;
                }
                default: break;
            }

            return n;
        }
        ast_node* stringLiteral(Parser* ps) {
            if (!ps->typeIs(tt_string)) return nullptr;
            const token& t = ps->get();
            ps->consume();
            ast_node* n = ps->newNode(nt_literal, &t);
            n->value_tp = lt_string;
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            return n;
        }
        ast_node* templateStringLiteral(Parser* ps) {
            if (!ps->typeIs(tt_template_string)) return nullptr;
            // todo...

            const token& t = ps->get();
            ps->consume();
            ast_node* n = ps->newNode(nt_literal, &t);
            n->value_tp = lt_string;
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            return n;
        }
        ast_node* arrayLiteral(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_bracket)) return nullptr;
            ps->push();
            ps->consume();

            ast_node* n = expressionSequence(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_close_bracket)) {
                ps->error(pec_expected_closing_bracket, "Expected ']' to close array literal");
                ps->freeNode(n);
                return nullptr;
            }
            n->manuallySpecifyRange(ps->get());
            ps->consume();
            ps->commit();

            ast_node* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_array;
            a->body = n;
            return a;
        }
        ast_node* objectLiteralProperty(Parser* ps, bool expected) {
            const token* tok = &ps->get();
            ps->push();
            ast_node* name = identifier(ps);
            if (!name) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                if (expected) {
                    ps->error(pec_expected_colon, utils::String::Format("Expected ':' after '%s'", name->str().c_str()));
                }
                ps->revert();
                ps->freeNode(name);
                return nullptr;
            }

            ast_node* n = ps->newNode(nt_object_literal_property, tok);
            n->lvalue = name;
            n->rvalue = singleExpression(ps);
            if (!n->rvalue) {
                ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%s:'", name->str().c_str()));
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }

            ps->commit();
            return n;
        }
        ast_node* objectLiteral(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->push();
            ps->consume();

            ast_node* f = objectLiteralProperty(ps, false);
            if (!f) {
                ps->revert();
                return nullptr;
            }
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();
                n->next = objectLiteralProperty(ps, true);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_object_property, "Expected object literal property after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_bracket, "Expected '}' to close object literal");
                ps->freeNode(n);
                return nullptr;
            }
            n->manuallySpecifyRange(ps->get());
            ps->consume();
            ps->commit();

            ast_node* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_object;
            a->body = f;
            return a;
        }
        
        
        
        //
        // Expressions
        //
        
        ast_node* primaryExpression(Parser* ps) {
            if (ps->isKeyword("this")) {
                ps->consume();
                return ps->newNode(nt_this, &ps->getPrev());
            }

            if (ps->isKeyword("null")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_null;
                return n;
            }

            if (ps->isKeyword("true")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_true;
                return n;
            }

            if (ps->isKeyword("false")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_false;
                return n;
            }

            if (ps->isKeyword("sizeof")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_sizeof, &ps->getPrev());

                if (!ps->isSymbol("<")) {
                    ps->error(pec_expected_symbol, "Expected single template parameter after 'sizeof'");
                    ps->freeNode(n);
                    return nullptr;
                }

                n->data_type = templateArgs(ps);
                if (!n->data_type) {
                    // error already emitted
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->data_type->next) {
                    ps->error(pec_expected_single_template_arg, "Expected single template parameter after 'sizeof'");
                    // attempt to continue
                }

                return n;
            }

            // todo:
            // hex literal
            // regex literal?

            ast_node* n = numberLiteral(ps);
            if (n) return n;

            n = stringLiteral(ps);
            if (n) return n;

            n = templateStringLiteral(ps);
            if (n) return n;

            n = arrayLiteral(ps);
            if (n) return n;

            n = objectLiteral(ps);
            if (n) return n;

            n = identifier(ps);
            if (n) {
                if (ps->isSymbol("<")) {
                    ps->push();
                    ast_node* args = templateArgs(ps);
                    
                    if (args) {
                        // it's either a type specifier or a function specialization
                        ps->commit();
                        n->template_parameters = args;
                        return n;
                    } else ps->revert();
                }
                
                return n;
            }

            n = expressionSequenceGroup(ps);
            if (n) return n;

            return nullptr;
        }
        ast_node* functionExpression(Parser* ps) {
            const token* tok = &ps->get();
            ps->push();
            ast_node* params = one_of(ps, { identifier, maybeParameterList });
            if (!params) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_forward)) {
                ps->revert();
                ps->freeNode(params);
                return nullptr;
            }
            ps->commit();
            ps->consume();

            ast_node* n = ps->newNode(nt_function, tok);
            n->parameters = params;
            n->body = one_of(ps, { block, expression });

            if (!n->body) {
                ps->error(pec_expected_function_body, "Expected arrow function body");
                ps->freeNode(n);
                return nullptr;
            }

            return n;
        }
        ast_node* memberExpression(Parser* ps) {
            ast_node* n = functionExpression(ps);
            if (!n) n = primaryExpression(ps);
            if (!n) return nullptr;

            ast_node* l = n;
            while (true) {
                if (ps->typeIs(tt_dot)) {
                    const token& tok = ps->get();
                    ps->consume();

                    ast_node* r = identifier(ps);
                    if (!r) {
                        ps->error(pec_expected_identifier, "Expected identifier after '.'");
                        ps->freeNode(l);
                        return nullptr;
                    }

                    ast_node* e = ps->newNode(nt_expression, &tok);
                    e->op = op_member;
                    e->lvalue = l;
                    e->rvalue = r;
                    l = e;
                    continue;
                } else if (ps->typeIs(tt_open_bracket)) {
                    const token& tok = ps->get();
                    ps->consume();
                    
                    ast_node* r = expression(ps);
                    if (!r) {
                        ps->revert();
                        ps->error(pec_expected_expr, "Expected expression after '['");
                        ps->freeNode(l);
                        return nullptr;
                    }

                    if (!ps->typeIs(tt_close_bracket)) {
                        ps->revert();
                        ps->error(pec_expected_closing_bracket, "Expected ']'");
                        ps->freeNode(l);
                        return nullptr;
                    }

                    ast_node* e = ps->newNode(nt_expression, &tok);
                    e->op = op_index;
                    e->lvalue = l;
                    e->rvalue = r;

                    n = e;
                }

                break;
            }

            return l;
        }
        ast_node* callExpression(Parser* ps) {
            ps->push();
            ast_node* callee = memberExpression(ps);
            if (!callee) {
                ps->revert();
                return nullptr;
            }

            const token& argTok = ps->get();

            ast_node* args = arguments(ps);
            if (!args) {
                ps->revert();
                ps->freeNode(callee);
                return nullptr;
            }

            ast_node* n = ps->newNode(nt_expression, &argTok);
            n->op = op_call;
            n->lvalue = callee;
            n->parameters = args;

            while (true) {
                if (ps->typeIs(tt_open_parenth)) {
                    ast_node* e = ps->newNode(nt_expression, &ps->get());
                    e->op = op_call;
                    e->lvalue = n;
                    e->parameters = arguments(ps);

                    n = e;
                    continue;
                } else if (ps->typeIs(tt_dot)) {
                    ps->consume();
                    ast_node* e = ps->newNode(nt_expression, &ps->getPrev());
                    e->op = op_member;
                    e->lvalue = n;
                    e->rvalue = identifier(ps);
                    
                    if (!e->rvalue) {
                        ps->revert();
                        ps->error(pec_expected_identifier, "Expected identifier after '.'");
                        ps->freeNode(e);
                        return nullptr;
                    }

                    n = e;
                    continue;
                } else if (ps->typeIs(tt_open_bracket)) {
                    ps->consume();
                    ast_node* e = ps->newNode(nt_expression, &ps->getPrev());
                    e->op = op_index;
                    e->lvalue = n;
                    e->rvalue = expression(ps);
                    
                    if (!e->rvalue) {
                        ps->revert();
                        ps->error(pec_expected_expr, "Expected expression after '['");
                        ps->freeNode(e);
                        return nullptr;
                    }

                    if (!ps->typeIs(tt_close_bracket)) {
                        ps->revert();
                        ps->error(pec_expected_closing_bracket, "Expected ']'");
                        ps->freeNode(e);
                        return nullptr;
                    }

                    n = e;
                    continue;
                }

                break;
            }

            ps->commit();

            return n;
        }
        ast_node* leftHandSideExpression(Parser* ps) {
            ast_node* n = callExpression(ps);
            if (!n) n = memberExpression(ps);

            if (ps->isKeyword("as")) {
                ast_node* o = ps->newNode(nt_cast);
                ps->consume();

                o->body = n;
                o->data_type = typeSpecifier(ps);

                if (!o->data_type) {
                    ps->error(pec_expected_type_specifier, "Expected type specifier after 'as' keyword");
                    ps->freeNode(o);
                    return nullptr;
                }

                n = o;
            }

            return n;
        }
        ast_node* postfixExpression(Parser* ps) {
            ast_node* n = leftHandSideExpression(ps);
            if (!n) return nullptr;

            const token& t = ps->get();
            if (t.text.size() == 2 && (t.text[0] == '-' || t.text[0] == '+') && t.text[0] == t.text[1]) {
                // --, ++
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, &t);
                e->op = (t.text[0] == '-') ? op_postDec : op_postInc;
                e->lvalue = n;

                return e;
            }

            return n;
        }
        ast_node* unaryExpression(Parser* ps) {
            ast_node* n = postfixExpression(ps);
            if (n) return n;

            const token& t = ps->get();
            if (t.text.size() == 1 && (t.text[0] == '-' || t.text[0] == '~' || t.text[0] == '!')) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, &t);
                e->op = (t.text[0] == '-') ? op_negate : ((t.text[0] == '~') ? op_bitInv : op_not);
                e->lvalue = unaryExpression(ps);
                
                if (!e->lvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c'", t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            if (t.text.size() == 2 && (t.text[0] == '-' || t.text[0] == '+') && t.text[0] == t.text[1]) {
                // --, ++
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, &t);
                e->op = (t.text[0] == '-') ? op_preDec : op_preInc;
                e->lvalue = unaryExpression(ps);
                
                if (!e->lvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c%c'", t.text[0], t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return nullptr;
        }
        ast_node* multiplicativeExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = unaryExpression(ps);

            const token& t = ps->get();
            if (t.text.size() == 1 && (t.text[0] == '*' || t.text[0] == '/' || t.text[0] == '%')) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = (t.text[0] == '*') ? op_mul : ((t.text[0] == '/') ? op_div : op_mod);
                e->lvalue = n;
                e->rvalue = unaryExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c'", t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* additiveExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = multiplicativeExpression(ps);

            const token& t = ps->get();
            if (t.text.size() == 1 && (t.text[0] == '+' || t.text[0] == '-')) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = (t.text[0] == '+') ? op_add : op_sub;
                e->lvalue = n;
                e->rvalue = multiplicativeExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c'", t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* shiftExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = additiveExpression(ps);

            const token& t = ps->get();
            if (t.text.size() == 2 && (t.text[0] == '<' || t.text[0] == '>') && t.text[0] == t.text[1]) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = (t.text[0] == '<') ? op_shLeft : op_shRight;
                e->lvalue = n;
                e->rvalue = additiveExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c%c'", t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* relationalExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = shiftExpression(ps);

            const token& t = ps->get();
            if ((t.text[0] == '<' || t.text[0] == '>') && (t.text.size() == 1 || t.text[1] == '=')) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);

                const char* sym = nullptr;
                if (t.text[0] == '<') {
                    if (t.text.size() == 1) {
                        e->op = op_lessThan;
                        sym = "<";
                    } else {
                        e->op = op_lessThanEq;
                        sym = "<=";
                    }
                } else {
                    if (t.text.size() == 1) {
                        e->op = op_greaterThan;
                        sym = ">";
                    } else {
                        e->op = op_greaterThanEq;
                        sym = ">=";
                    }
                }

                e->lvalue = n;
                e->rvalue = shiftExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%s'", sym));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* equalityExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = relationalExpression(ps);

            const token& t = ps->get();
            if (t.text.size() == 2 && (t.text[0] == '!' || t.text[0] == '=')) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = (t.text[0] == '!') ? op_notEq : op_compare;
                e->lvalue = n;
                e->rvalue = relationalExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%c='", t.text[0]));
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* bitwiseAndExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = equalityExpression(ps);

            if (ps->textIs("&")) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = op_bitAnd;
                e->lvalue = n;
                e->rvalue = equalityExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '&'");
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* XOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = bitwiseAndExpression(ps);

            if (ps->textIs("^")) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = op_xor;
                e->lvalue = n;
                e->rvalue = bitwiseAndExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '^'");
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* bitwiseOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = XOrExpression(ps);

            if (ps->textIs("|")) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = op_bitOr;
                e->lvalue = n;
                e->rvalue = XOrExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '|'");
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* logicalAndExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = bitwiseOrExpression(ps);

            if (ps->textIs("&&")) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = op_logAnd;
                e->lvalue = n;
                e->rvalue = bitwiseOrExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '&&'");
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* logicalOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = logicalAndExpression(ps);

            if (ps->textIs("||")) {
                ps->consume();

                ast_node* e = ps->newNode(nt_expression, first);
                e->op = op_logOr;
                e->lvalue = n;
                e->rvalue = logicalAndExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '||'");
                    ps->freeNode(e);
                    return nullptr;
                }

                return e;
            }

            return n;
        }
        ast_node* conditionalExpression(Parser* ps) {
            const token* first = &ps->get();
            ast_node* n = logicalOrExpression(ps);

            if (ps->get().text[0] == '?') {
                ps->consume();
                ast_node* c = ps->newNode(nt_expression, first);
                c->op = op_conditional;
                c->cond = n;
                c->lvalue = assignmentExpression(ps);

                if (!c->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression for conditional expression's truth result");
                    ps->freeNode(c);
                    return nullptr;
                }

                if (!ps->typeIs(tt_colon)) {
                    ps->error(pec_expected_symbol, "Expected ':' after conditional expression's truth result");
                    ps->freeNode(c);
                    return nullptr;
                }
                ps->consume();
                c->rvalue = assignmentExpression(ps);

                if (!c->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression for conditional expression's false result");
                    ps->freeNode(c);
                    return nullptr;
                }

                return c;
            }

            return n;
        }
        ast_node* assignmentExpression(Parser* ps) {
            const token* first = &ps->get();
            ps->push();
            ast_node* lvalue = leftHandSideExpression(ps);
            if (lvalue) {
                if (isAssignmentOperator(ps)) {
                    expr_operator op = getOperatorType(ps);
                    ps->consume();
                    ast_node* rvalue = assignmentExpression(ps);

                    if (rvalue) {
                        ps->commit();

                        ast_node* out = ps->newNode(nt_expression, first);
                        out->op = op;
                        out->lvalue = lvalue;
                        out->rvalue = rvalue;
                        return out;
                    }
                }
            }

            ps->revert();
            return conditionalExpression(ps);
        }
        ast_node* singleExpression(Parser* ps) {
            return assignmentExpression(ps);
        }
        ast_node* expression(Parser* ps) {
            ast_node* n = assignmentExpression(ps);
            ast_node* b = n;
            while (ps->typeIs(tt_comma)) {
                ps->consume();

                b->next = assignmentExpression(ps);
                if (!b->next) {
                    ps->error(pec_expected_expr, "Expected expression after ','");
                    // attempt to continue
                    break;
                }

                b = b->next;
            }

            return n;
        }
        ast_node* expressionSequence(Parser* ps) {
            ast_node* f = singleExpression(ps);
            if (!f) return nullptr;
            
            ast_node* n = f;
            while (true) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = singleExpression(ps);
                n = n->next;

                if (!n) {
                    ps->error(pec_expected_expr, "Expected expression after ','");
                    ps->freeNode(f);
                    return nullptr;
                }
            }

            return f;
        }
        ast_node* expressionSequenceGroup(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->push();
            ps->consume();
            
            ast_node* n = expressionSequence(ps);
            
            if (!n) {
                ps->revert();
                return nullptr;
            }

            ps->commit();
            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')'");
                // attempt to continue
                return n;
            }

            ps->consume();

            return n;
        }
        
        
        //
        // Declarations
        //
        ast_node* variableDecl(Parser* ps) {
            ast_node* _assignable = one_of(ps, { typedAssignable, assignable, objectDecompositor });
            if (!_assignable) return nullptr;

            ast_node* decl = ps->newNode(nt_variable, &_assignable->tok);
            decl->body = _assignable;
            if (ps->isSymbol("=")) {
                ps->consume();
                decl->initializer = singleExpression(ps);
                if (!decl->initializer) {
                    ps->error(pec_expected_expr, "Expected expression for variable initializer");
                }
            }

            return decl;
        }
        ast_node* variableDeclList(Parser* ps) {
            bool isConst = ps->isKeyword("const");
            if (!isConst && !ps->isKeyword("let")) return nullptr;
            const token* tok = &ps->get();
            ps->push();
            ps->consume();

            ast_node* f = variableDecl(ps);
            if (!f) {
                ps->error(pec_expected_variable_decl, "Expected variable declaration");
                ps->revert();
                ps->freeNode(f);
                return nullptr;
            }

            f->flags.is_const = isConst ? 1 : 0;
            f->tok = *tok;

            ps->commit();

            ast_node* n = f;
            while (true) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = variableDecl(ps);
                n = n->next;

                if (!n) {
                    ps->error(pec_expected_variable_decl, "Expected variable declaration");
                    ps->revert();
                    ps->freeNode(f);
                    return nullptr;
                }

                n->flags.is_const = isConst ? 1 : 0;
            }

            return f;
        }
        ast_node* classPropertyOrMethod(Parser* ps) {
            access_modifier am = public_access;
            bool isStatic = false;
            const token& ft = ps->get();

            bool didCommit = false;
            ps->push();

            if (ps->isKeyword("public")) {
                ps->commit();
                didCommit = true;
                ps->consume();
            } else if (ps->isKeyword("private")) {
                ps->commit();
                didCommit = true;
                am = private_access;
                ps->consume();
            }

            if (ps->isKeyword("static")) {
                ps->commit();
                didCommit = true;
                isStatic = true;
            }

            bool isGetter = false;
            bool isSetter = false;

            if (ps->isKeyword("get")) {
                ps->commit();
                didCommit = true;
                isGetter = true;
                ps->consume();
            } else if (ps->isKeyword("set")) {
                ps->commit();
                didCommit = true;
                isSetter = true;
                ps->consume();
            }

            ast_node* n = identifier(ps);
            bool isOperator = false;
            bool isCastOperator = false;
            if (!n && ps->typeIs(tt_keyword) && ps->textIs("operator")) {
                if (isGetter || isSetter) {
                    ps->error(pec_reserved_word, "Cannot name a getter or setter method 'operator', 'operator' is a reserved word");
                    return nullptr;
                }

                // treat "operator" as an identifier for the method name
                n = ps->newNode(nt_identifier);
                const token& t = ps->get();
                n->value.s = t.text.c_str();
                n->str_len = t.text.size();
                ps->consume();
                n->op = getOperatorType(ps);
                if (n->op == op_undefined) {
                    n->data_type = typeSpecifier(ps);
                    if (!n->data_type) {
                        ps->error(pec_expected_operator_override_target, "Expected operator or type specifier after 'operator' keyword");
                        ps->freeNode(n);
                        return nullptr;
                    }

                    isCastOperator = true;
                } else {
                    ps->consume();
                    if (n->op == op_call || n->op == op_index) {
                        // actually two tokens
                        ps->consume();
                    }
                }

                isOperator = true;
            }

            if (!n) {
                if (isGetter) {
                    ps->error(pec_expected_identifier, "Expected property name for getter method");
                } else if (isSetter) {
                    ps->error(pec_expected_identifier, "Expected property name for setter method");
                } else if (didCommit) {
                    ps->error(pec_expected_identifier, "Expected property or method name");
                }

                ps->revert();
                return nullptr;
            }

            if (!didCommit) {
                ps->commit();
                didCommit = true;
            }
            
            n->flags.is_private = (am == private_access) ? 1 : 0;
            n->flags.is_static = isStatic ? 1 : 0;
            n->flags.is_getter = isGetter ? 1 : 0;
            n->flags.is_setter = isSetter ? 1 : 0;
            n->tok = ft;

            if (!isGetter && !isSetter && ps->typeIs(tt_colon)) {
                if (isOperator) {
                    ps->error(pec_reserved_word, "Cannot name a class property 'operator', 'operator' is a reserved word");
                    ps->freeNode(n);
                    return nullptr;
                }

                ps->consume();
                n->tp = nt_property;
                n->data_type = typeSpecifier(ps);
                if (!n->data_type) {
                    ps->error(pec_expected_type_specifier, "Expected type specifier for class property");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after property declaration");
                    // attempt to continue
                } else ps->consume();

                return n;
            }

            ast_node* templArgs = nullptr;
            if (ps->isSymbol("<")) {
                templArgs = templateParams(ps);
                if (!templArgs) {
                    // error already emitted
                    ps->freeNode(n);
                    return nullptr;
                }
            }

            if (ps->typeIs(tt_open_parenth)) {
                n->tp = nt_function;
                n->parameters = typedParameterList(ps);
                n->template_parameters = templArgs;

                if (ps->typeIs(tt_colon)) {
                    if (isCastOperator) {
                        ps->error(pec_unexpected_type_specifier, "Cast operator overrides should not have return types specified");
                        // attempt to continue
                        ps->freeNode(n->data_type); // (was already allocated)
                        n->data_type = nullptr;
                    } else if (n->str() == "constructor") {
                        ps->error(pec_unexpected_type_specifier, "Class constructors should not have return types specified");
                        // attempt to continue
                    } else if (n->str() == "destructor") {
                        ps->error(pec_unexpected_type_specifier, "Class destructors should not have return types specified");
                        // attempt to continue
                    }

                    ps->consume();
                    n->data_type = typeSpecifier(ps);
                    if (!n->data_type) {
                        if (!isCastOperator && n->str() != "constructor" && n->str() == "destructor") ps->error(pec_expected_type_specifier, "Expected type specifier for return value of class method");
                        ps->freeNode(n);
                        return nullptr;
                    }
                }

                n->body = block(ps);
                if (!n->body) {
                    ps->error(pec_expected_function_body, "Expected function body after class method declaration");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            } else if (isOperator || templArgs) {
                // Only possibility is that it was supposed to be a method
                ps->error(pec_expected_parameter_list, "Expected parameter list for operator override method");
                ps->freeNode(n);
                return nullptr;
            } else if (isGetter) {
                ps->error(pec_expected_identifier, "Expected class property getter method");
                ps->freeNode(n);
                return nullptr;
            } else if (isSetter) {
                ps->error(pec_expected_identifier, "Expected class property setter method");
                ps->freeNode(n);
                return nullptr;
            }
        
            ps->freeNode(n);
            ps->error(pec_malformed_class_element, "Expected property type specifier or method parameter list after identifier");
            return nullptr;
        }
        ast_node* classDef(Parser* ps) {
            if (!ps->isKeyword("class")) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ast_node* n = identifier(ps);
            n->tok = ft;
            n->tp = nt_class;
            n->template_parameters = templateParams(ps);

            if (ps->isKeyword("extends")) {
                ps->consume();
                n->inheritance = list_of(
                    ps,
                    typeSpecifier,
                    pec_expected_type_specifier,
                    "Expected one or more type specifiers after 'extends'",
                    pec_expected_type_specifier,
                    "Expected type specifier after ','"
                );
                
                if (!n->inheritance) {
                    // error already emitted
                    ps->freeNode(n);
                    return nullptr;
                }
            }

            if (!ps->typeIs(tt_open_brace)) {
                ps->error(pec_expected_open_brace, "Expected '{' after class name");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            n->body = array_of(ps, classPropertyOrMethod);

            if (!n->body) {
                ps->error(pec_empty_class, "Class body cannot be empty");
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_open_brace, "Expected '}' after class body");
                ps->freeNode(n);
                return nullptr;
            }
            n->manuallySpecifyRange(ps->get());
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after class definition");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* typeDef(Parser* ps) {
            if (!ps->isKeyword("type")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* n = identifier(ps);

            n->template_parameters = templateParams(ps);

            if (!n) {
                ps->error(pec_expected_identifier, "Expected identifier for type name");
                return nullptr;
            }

            if (!ps->isSymbol("=")) {
                ps->error(pec_expected_symbol, utils::String::Format("Expected '=' after 'type %s'", n->str().c_str()));
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            n->tp = nt_type;
            n->tok = *t;
            n->data_type = typeSpecifier(ps);
            if (!n->data_type) {
                ps->error(pec_expected_type_specifier, utils::String::Format("Expected type specifier after 'type %s ='", n->str().c_str()));
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after type definition");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* functionDecl(Parser* ps) {
            if (!ps->isKeyword("function")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* n = identifier(ps);
            if (!n) {
                ps->error(pec_expected_identifier, "Expected identifier after 'function'");
                return nullptr;
            }


            n->tp = nt_function;
            n->tok = *t;
            n->template_parameters = templateParams(ps);
            n->parameters = parameterList(ps);
            if (!n->parameters) {
                ps->error(pec_expected_parameter_list, "Expected function parameter list");
                ps->freeNode(n);
                return nullptr;
            }

            return n;
        }
        ast_node* functionDef(Parser* ps) {
            ast_node* n = functionDecl(ps);
            if (!n) return nullptr;

            n->body = block(ps);

            if (!n->body) {
                ps->error(pec_expected_eos, "Expected function body");
            }

            return n;
        }
        ast_node* declaration(Parser* ps) {
            return one_of(ps, {
                variableStatement,
                classDef,
                typeDef,
                functionDef
            });
        }
        
        
        
        //
        // Statements
        //
        ast_node* variableStatement(Parser* ps) {
            return all_of(ps, { variableDeclList, eosRequired });
        }
        ast_node* emptyStatement(Parser* ps) {
            return eos(ps);
        }
        ast_node* expressionStatement(Parser* ps) {
            return all_of(ps, { expressionSequence, eosRequired });
        }
        ast_node* ifStatement(Parser* ps) {
            if (!ps->isKeyword("if")) return nullptr;
            ps->consume();
            ast_node* n = ps->newNode(nt_if, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pec_expected_expgroup, "Expected '(<expression>)' after 'if'");
                ps->freeNode(n);
                return nullptr;
            }

            n->body = statement(ps);
            if (!n->body) {
                ps->error(pec_expected_statement, "Expected block or statement after 'if (<expression>)'");
                ps->freeNode(n);
                return nullptr;
            }

            if (n->body->tp != nt_scoped_block) {
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after 'if (<expression>) statement'");
                    // attempt to continue
                } else {
                    n->manuallySpecifyRange(ps->get());
                    ps->consume();
                }
            }
            
            if (ps->isKeyword("else")) {
                ps->consume();
                n->else_body = statement(ps);
                if (!n->else_body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'else'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->else_body->tp != nt_scoped_block) {
                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pec_expected_eos, "Expected ';' after 'else statement'");
                        // attempt to continue
                    } else {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                    }
                }
            }

            return n;
        }
        ast_node* continueStatement(Parser* ps) {
            if (!ps->isKeyword("continue")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after 'continue'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ast_node* breakStatement(Parser* ps) {
            if (!ps->isKeyword("break")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after 'break'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ast_node* iterationStatement(Parser* ps) {
            if (ps->isKeyword("do")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_loop, &ps->getPrev());
                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'do'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->isKeyword("while")) {
                    ps->error(pec_expected_while, "Expected 'while' after 'do ...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->cond = expressionSequenceGroup(ps);
                if (!n->cond) {
                    ps->error(pec_expected_expgroup, "Expected '(<expression>)' after 'do ... while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after 'do ... while (<expression>)'");
                    // attempt to continue
                } else {
                    if (n->body->tp != nt_scoped_block) n->manuallySpecifyRange(ps->get());
                    ps->consume();
                }
                
                n->flags.defer_cond = 1;
                return n;
            } else if (ps->isKeyword("while")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_loop, &ps->getPrev());
                n->cond = expressionSequenceGroup(ps);
                if (!n->cond) {
                    ps->error(pec_expected_expgroup, "Expected '(<expression>)' after 'while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'while (<expression>)'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->body->tp != nt_scoped_block) {
                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pec_expected_eos, "Expected ';' after 'while (<expression>) statement'");
                        // attempt to continue
                    } else {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                    }
                }

                return n;
            } else if (ps->isKeyword("for")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_loop, &ps->getPrev());

                if (!ps->typeIs(tt_open_parenth)) {
                    ps->error(pec_expected_open_parenth, "Expected '(' after 'for'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->initializer = one_of(ps, { expressionSequence, variableDeclList });

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after 'for (...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->cond = expressionSequence(ps);

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after 'for (...;...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->modifier = expressionSequence(ps);
                
                if (!ps->typeIs(tt_close_parenth)) {
                    ps->error(pec_expected_closing_parenth, "Expected ')' after 'for (...;...;...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'for (...;...;...)'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->body->tp != nt_scoped_block) {
                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pec_expected_eos, "Expected ';' after 'for (...;...;...) statement'");
                        // attempt to continue
                    } else {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                    }
                }

                return n;
            }

            return nullptr;
        }
        ast_node* returnStatement(Parser* ps) {
            if (!ps->isKeyword("return")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_return, &ps->getPrev());
            n->body = singleExpression(ps);

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after return statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* switchCase(Parser* ps) {
            if (!ps->isKeyword("case")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_switch_case, &ps->getPrev());
            n->cond = expressionSequence(ps);
            if (!n->cond) {
                ps->error(pec_expected_expgroup, "Expected '(<expression>)' after 'case'");
                ps->freeNode(n);
                return nullptr;
            }
            
            if (!ps->typeIs(tt_colon)) {
                ps->error(pec_expected_colon, "Expected ':' after 'case <expression>'");
                // attempt to continue
            } else ps->consume();

            n->body = statement(ps);

            return n;
        }
        ast_node* switchStatement(Parser* ps) {
            if (!ps->isKeyword("switch")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_return, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pec_expected_expgroup, "Expected '(<expression>)' after 'switch'");
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_open_brace)) {
                ps->error(pec_expected_open_brace, "Expected '{' after 'switch (<expression>)'");
                // attempt to continue
            } else ps->consume();

            n->body = array_of(ps, { switchCase });

            if (ps->isKeyword("default")) {
                ps->consume();

                if (!ps->typeIs(tt_colon)) {
                    ps->error(pec_expected_colon, "Expected ':' after 'switch (<expression>) { ... default'");
                    // attempt to continue
                } else ps->consume();

                n->else_body = statement(ps);
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_brace, "Expected '}' after 'switch (<expression>) { ...'");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* throwStatement(Parser* ps) {
            if (!ps->isKeyword("throw")) return nullptr;
            const token* t = &ps->get();
            ps->consume();
            ast_node* expr = singleExpression(ps);
            if (!expr) {
                ps->error(pec_expected_expr, "Expected expression after 'throw'");
                return nullptr;
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after 'throw <expression>'");
                // attempt to continue
            } else ps->consume();

            ast_node* n = ps->newNode(nt_throw, t);
            n->body = expr;
            return n;
        }
        ast_node* catchBlock(Parser* ps) {
            if (!ps->isKeyword("catch")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            if (!ps->typeIs(tt_open_parenth)) {
                ps->error(pec_expected_open_parenth, "Expected '(' after 'catch'");
                return nullptr;
            }
            ps->consume();

            ast_node* n = ps->newNode(nt_catch, t);
            n->parameters = parameter(ps);
            if (!n->parameters) {
                ps->error(pec_expected_catch_param, "Expected 'catch' block to take exactly one parameter");
                ps->freeNode(n);
                return nullptr;
            }

            if (!n->parameters->data_type) {
                ps->error(pec_expected_catch_param_type, "Catch block parameter must be explicitly typed");
                ps->freeNode(n);
                return nullptr;
            }

            n->body = block(ps);
            if (!n->body) {
                // expect semicolon
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after empty 'catch' statement");
                    // attempt to continue
                } else ps->consume();
            }

            return n;
        }
        ast_node* tryStatement(Parser* ps) {
            if (!ps->isKeyword("try")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* body = block(ps);
            if (!body) {
                ps->error(pec_expected_statement, "Expected block or statement after 'try'");
                return nullptr;
            }

            ast_node* n = ps->newNode(nt_try, t);
            n->body = body;
            n->else_body = catchBlock(ps);
            if (!n->else_body) {
                ps->error(pec_expected_catch, "Expected at least one 'catch' block after 'try { ... }'");
                ps->freeNode(n);
                return nullptr;
            }

            ast_node* c = n->else_body;
            while (c) {
                c->next = catchBlock(ps);
                c = c->next;
            }

            return n;
        }
        ast_node* placementNewStatement(Parser* ps) {
            if (!ps->isKeyword("new")) return nullptr;
            const token* first = &ps->get();
            ps->push();
            ps->consume();

            ast_node* type = typeSpecifier(ps);
            if (!type) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_open_parenth)) {
                ps->revert();
                ps->freeNode(type);
                return nullptr;
            }

            ast_node* args = arguments(ps);

            if (!ps->typeIs(tt_forward)) {
                ps->revert();
                ps->freeNode(type);
                if (args) ps->freeNode(args);
                return nullptr;
            }

            ps->commit();
            ps->consume();

            ast_node* n = ps->newNode(nt_expression, first);
            n->op = op_placementNew;
            n->data_type = type;
            n->parameters = args;
            n->lvalue = singleExpression(ps);

            if (!n->lvalue) {
                ps->error(pec_expected_expr, "Expected expression for placement new target");
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after placement new statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* symbolImport(Parser* ps) {
            ast_node* n = one_of(ps, {
                typedIdentifier,
                identifier
            });
            if (!n) return nullptr;
            n->tp = nt_import_symbol;
            return n;
        }
        ast_node* importList(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->consume();

            ast_node* f = symbolImport(ps);
            ast_node* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = symbolImport(ps);
                n = n->next;
                if (!n) {
                    ps->error(pec_expected_parameter, "Expected symbol to import after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_parenth, "Expected '}' to close import list");
                ps->freeNode(f);
                return nullptr;
            }
            ps->consume();

            return f;
        }
        ast_node* importModule(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->push();
            ps->consume();

            if (!ps->isSymbol("*")) {
                ps->revert();
                return nullptr;
            }
            ps->consume();
            ps->commit();

            if (!ps->isKeyword("as")) {
                ps->error(pec_expected_import_alias, "Expected 'as' after 'import { *'");
                return nullptr;
            }
            ps->consume();

            ast_node* n = identifier(ps);
            if (!n) {
                ps->error(pec_expected_identifier, "Expected identifier for module alias");
                return nullptr;
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_parenth, "Expected '}' to close import list");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            n->tp = nt_import_module;
            return n;
        }
        ast_node* importStatement(Parser* ps) {
            if (!ps->isKeyword("import")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_import, &ps->getPrev());
            n->body = importModule(ps);
            if (!n->body) n->body = importList(ps);
            if (!n->body) {
                ps->error(pec_expected_import_list, "Expected import list after 'import'");
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->isKeyword("from")) {
                ps->error(pec_expected_import_name, "Expected 'from'");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            if (!ps->typeIs(tt_string)) {
                ps->error(pec_expected_import_name, "Expected string literal module name or file path after 'from'");
                ps->freeNode(n);
                return nullptr;
            }
            const token& t = ps->get();
            ps->consume();

            n->value.s = t.text.c_str();
            n->str_len = t.text.size();

            return n;
        }
        ast_node* exportStatement(Parser* ps) {
            if (!ps->isKeyword("export")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* decl = declaration(ps);
            if (!decl) {
                ps->error(pec_expected_export_decl, "Expected function, class, type, or variable declaration after 'export'");
                return nullptr;
            }

            ast_node* e = ps->newNode(nt_export, t);
            e->body = decl;
            return e;
        }
        ast_node* statement(Parser* ps) {
            ast_node* n = one_of(ps, {
                block,
                variableStatement,
                importStatement,
                exportStatement,
                emptyStatement,
                classDef,
                typeDef,
                placementNewStatement,
                expressionStatement,
                ifStatement,
                iterationStatement,
                continueStatement,
                breakStatement,
                returnStatement,
                switchStatement,
                throwStatement,
                tryStatement,
                functionDef
            });

            return n;
        }
        ast_node* statementList(Parser* ps) {
            return array_of(ps, statement);
        }
        ast_node* block(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->consume();

            ast_node* b = ps->newNode(nt_scoped_block, &ps->getPrev());
            b->body = statementList(ps);
            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_brace, "Expected '}'");
                ps->freeNode(b);
                return nullptr;
            }
            b->manuallySpecifyRange(ps->get());
            ps->consume();

            return b;
        }
        
        
        
        //
        // Entry
        //
        ast_node* program(Parser* ps) {
            ast_node* statements = array_of(ps, statement);

            if (!ps->typeIs(tt_eof)) {
                ps->error(pec_expected_eof, "Expected end of file");
                if (statements) ps->freeNode(statements);
                return nullptr;
            }
            ps->consume();

            if (!statements) return nullptr;
            
            ast_node* root = ps->newNode(nt_root, &statements->tok);
            root->body = statements;
            return root;
        }
    };
};