#include <gs/compiler/parser.h>
#include <gs/compiler/lexer.h>
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

        void ast_node::json(u32 indent, u32 index, bool noIndentOpenBrace) const {
            static const char* nts[] = {
                "empty",
                "root",
                "break",
                "catch",
                "class",
                "continue",
                "eos",
                "export",
                "expression",
                "function",
                "function_type",
                "identifier",
                "if",
                "import",
                "import_symbol",
                "import_module",
                "literal",
                "loop",
                "object_decompositor",
                "object_literal_property",
                "parameter",
                "property",
                "return",
                "scoped_block",
                "switch",
                "switch_case",
                "this",
                "throw",
                "try",
                "type",
                "type_modifier",
                "type_property",
                "type_specifier",
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
                "bitInv",
                "bitInvEq",
                "bitAnd",
                "bitAndEq",
                "bitOr",
                "bitOrEq",
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
                "conditional",
                "preInc",
                "postInc",
                "preDec",
                "postDec",
                "negate",
                "member",
                "index",
                "new",
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
                "array"
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
            if (tok && tok->src.isValid()) {
                utils::String ln = tok->src.getSource()->getLine(tok->src.getLine()).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");
                iprintf(indent, "\"source\": {\n");
                iprintf(indent + 1, "\"line\": %d,\n", tok->src.getLine());
                iprintf(indent + 1, "\"col\": %d,\n", tok->src.getCol());
                iprintf(indent + 1, "\"line_text\": \"%s\",\n", ln.c_str());
                utils::String indxStr = "";
                for (u32 i = 0;i < tok->src.getCol();i++) indxStr += ' ';
                indxStr += '^';
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
                    }
                }
            } else if (str_len > 0) {
                printf(",\n");
                iprintf(indent, "\"text\": \"%s\"", str().c_str());
            }
            
            if (tp == nt_expression) {
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
        


        //
        // ParserState
        //
        
        ParserState::ParserState(Lexer* l) : m_tokens(l->tokenize()), m_currentIdx(32), m_nodeAlloc(1024, 1024, true) {
            m_currentIdx.push(0);
        }

        ParserState::~ParserState() {
        }

        void ParserState::push() {
            m_currentIdx.push(m_currentIdx[m_currentIdx.size() - 1]);
        }

        void ParserState::revert() {
            m_currentIdx.pop();
        }

        void ParserState::commit() {
            u32 idx = m_currentIdx.pop();
            m_currentIdx[m_currentIdx.size() - 1] = idx;
        }

        void ParserState::consume() {
            m_currentIdx[m_currentIdx.size() - 1]++;
        }

        const token& ParserState::get() const {
            return m_tokens[m_currentIdx[m_currentIdx.size() - 1]];
        }

        const token& ParserState::getPrev() const {
            u32 idx = m_currentIdx[m_currentIdx.size() - 1];
            return m_tokens[idx > 0 ? idx - 1 : 0];
        }

        ast_node* ParserState::parse() {
            m_nodeAlloc.reset();
            return program(this);
        }

        ast_node* ParserState::newNode(node_type tp, const token* src) {
            ast_node* n = m_nodeAlloc.alloc();
            n->tok = src ? src : &get();
            n->tp = tp;
            return n;
        }

        void ParserState::freeNode(ast_node* n) {
            if (n->data_type) freeNode(n->data_type);
            if (n->lvalue) freeNode(n->lvalue);
            if (n->rvalue) freeNode(n->rvalue);
            if (n->cond) freeNode(n->cond);
            if (n->body) freeNode(n->body);
            if (n->else_body) freeNode(n->else_body);
            if (n->initializer) freeNode(n->initializer);
            if (n->parameters) freeNode(n->parameters);
            if (n->modifier) freeNode(n->modifier);
            if (n->alias) freeNode(n->alias);
            if (n->next) freeNode(n->next);
            m_nodeAlloc.free(n);
        }

        const utils::Array<parse_error>& ParserState::errors() const {
            return m_errors;
        }

        bool ParserState::typeIs(token_type tp) const {
            return get().tp == tp;
        }

        bool ParserState::textIs(const char* str) const {
            return get().text == str;
        }

        bool ParserState::isKeyword(const char* str) const {
            const token& t = get();
            return t.tp == tt_keyword && t.text == str;
        }

        bool ParserState::isSymbol(const char* str) const {
            const token& t = get();
            return t.tp == tt_symbol && t.text == str;
        }

        void ParserState::error(parse_error_code code, const utils::String& msg) {
            error(code, msg, get());
        }
        
        void ParserState::error(parse_error_code code, const utils::String& msg, const token& tok) {
            m_errors.push({
                code,
                msg,
                tok
            });
        }



        //
        // Helpers
        //
        typedef ast_node* (*parser_fn)(ParserState*);
        ast_node* array_of(ParserState* ps, parser_fn fn) {
            ast_node* f = fn(ps);
            ast_node* n = f;
            while (n) {
                n->next = fn(ps);
                n = n->next;
            }
            return f;
        }
        ast_node* one_of(ParserState* ps, std::initializer_list<parser_fn> rules) {
            for (auto fn : rules) {
                ast_node* n = fn(ps);
                if (n) return n;
            }

            return nullptr;
        }
        ast_node* all_of(ParserState* ps, std::initializer_list<parser_fn> rules) {
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



        //
        // Parser logic
        //
        ast_node* eos(ParserState* ps) {
            if (!ps->typeIs(tt_semicolon)) return nullptr;
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ast_node* eosRequired(ParserState* ps) {
            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';'");
                return nullptr;
            }
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ast_node* statementList(ParserState* ps) {
            return array_of(ps, statement);
        }
        ast_node* block(ParserState* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->consume();

            ast_node* b = ps->newNode(nt_scoped_block, &ps->getPrev());
            b->body = statementList(ps);
            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pec_expected_closing_brace, "Expected '}'");
                ps->freeNode(b);
                return nullptr;
            }
            ps->consume();

            return b;
        }
        ast_node* identifier(ParserState* ps) {
            if (!ps->typeIs(tt_identifier)) return nullptr;
            ast_node* n = ps->newNode(nt_identifier);
            const token& t = ps->get();
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            ps->consume();
            return n;
        }
        ast_node* typedIdentifier(ParserState* ps) {
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
        ast_node* typeModifier(ParserState* ps) {
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
        ast_node* typeProperty(ParserState* ps) {
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
        ast_node* parenthesizedTypeSpecifier(ParserState* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token* t = &ps->get();
            ps->push();
            ps->consume();

            ast_node* n = typeSpecifier(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            n->tok = t;

            ps->commit();

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pec_expected_closing_parenth, "Expected ')'");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            return n;
        }
        ast_node* typeSpecifier(ParserState* ps) {
            const token* tok = &ps->get();
            ast_node* id = identifier(ps);
            if (id) {
                id->modifier = typeModifier(ps);

                ast_node* n = ps->newNode(nt_type_specifier, tok);
                n->body = id;
                return id;
            }

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
        ast_node* assignable(ParserState* ps) {
            return identifier(ps);
        }
        ast_node* typedAssignable(ParserState* ps) {
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
        ast_node* parameter(ParserState* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ast_node* typedParameter(ParserState* ps) {
            return typedAssignable(ps);
        }
        ast_node* maybeTypedParameterList(ParserState* ps) {
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
        ast_node* maybeParameterList(ParserState* ps) {
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
        ast_node* parameterList(ParserState* ps) {
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
        ast_node* arguments(ParserState* ps) {
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
        ast_node* objectDecompositorProperty(ParserState* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ast_node* objectDecompositor(ParserState* ps) {
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

                n->next = parameter(ps);
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
        ast_node* anonymousFunction(ParserState* ps) {
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
        
        ast_node* numberLiteral(ParserState* ps) {
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
        ast_node* stringLiteral(ParserState* ps) {
            if (!ps->typeIs(tt_string)) return nullptr;
            const token& t = ps->get();
            ps->consume();
            ast_node* n = ps->newNode(nt_literal, &t);
            n->value_tp = lt_string;
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            return n;
        }
        ast_node* templateStringLiteral(ParserState* ps) {
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
        ast_node* arrayLiteral(ParserState* ps) {
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
            ps->consume();
            ps->commit();

            ast_node* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_array;
            a->body = n;
            return a;
        }
        ast_node* objectLiteralProperty(ParserState* ps, bool expected) {
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
            n->rvalue = expression(ps);
            if (!n->rvalue) {
                ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%s:'", name->str().c_str()));
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }

            ps->commit();
            return n;
        }
        ast_node* objectLiteral(ParserState* ps) {
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
            ps->consume();
            ps->commit();

            ast_node* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_object;
            a->body = f;
            return a;
        }

        ast_node* primaryExpression(ParserState* ps) {
            if (ps->isKeyword("this")) {
                ps->consume();
                return ps->newNode(nt_this, &ps->getPrev());
            }

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
            if (n) return n;

            n = expressionSequenceGroup(ps);
            if (n) return n;

            return nullptr;
        }
        ast_node* opAssignmentExpression(ParserState* ps) {
            ast_node* n = primaryExpression(ps);
            if (!n || !ps->typeIs(tt_symbol)) return n;
            const token& t = ps->get();
            char c = t.text[0];
            expr_operator op = op_undefined;
            char nextMustMatch = '=';
            switch (c) {
                case '+': { op = op_addEq; break; }
                case '-': { op = op_subEq; break; }
                case '*': { op = op_mulEq; break; }
                case '/': { op = op_divEq; break; }
                case '~': { op = op_bitInvEq; break; }
                case '%': { op = op_modEq; break; }
                case '^': { op = op_xorEq; break; }
                case '&': {
                    if (t.text.size() == 2) {
                        op = op_bitAndEq;
                    } else {
                        op = op_logAndEq;
                        nextMustMatch = c;
                    }
                    break;
                }
                case '|': {
                    if (t.text.size() == 2) {
                        op = op_bitOrEq;
                    } else {
                        op = op_logOrEq;
                        nextMustMatch = c;
                    }
                    break;
                }
                case '<': {
                    if (t.text.size() == 2) {
                        // It's << or <=, not an assignment
                        return n;
                    } else {
                        op = op_shLeftEq;
                        nextMustMatch = c;
                    }
                    break;
                }
                case '>': {
                    if (t.text.size() == 2) {
                        // It's >> or >=, not an assignment
                        return n;
                    } else {
                        op = op_shRightEq;
                        nextMustMatch = c;
                    }
                    break;
                }
                default: return n;
            }

            // X=
            if (t.text[1] != nextMustMatch) return n;

            // XX=
            if (nextMustMatch != '=' && t.text[2] != '=') return n;

            // No turning back, it's definitely an assignment operator
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, utils::String::Format("Expected expression after '%s'", t.text.c_str()));
                ps->freeNode(o);
                return nullptr;
            }

            return o;
        }
        ast_node* assignmentExpression(ParserState* ps) {
            ast_node* n = opAssignmentExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("=")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_assign;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '='");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* conditionalExpression(ParserState* ps) {
            ast_node* n = assignmentExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("?")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_conditional;
            o->cond = n;
            o->lvalue = expression(ps);
            if (!o->lvalue) {
                ps->error(pec_expected_expr, "Expected expression after '?'");
                ps->freeNode(o);
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->error(pec_expected_colon, "Expected ':' after '<expression> ? <expression>");
                ps->freeNode(o);
                return nullptr;
            }
            ps->consume();

            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '<expression> ? <expression> : '");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* logicalOrExpression(ParserState* ps) {
            ast_node* n = conditionalExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("||")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_logOr;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '||'");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* logicalAndExpression(ParserState* ps) {
            ast_node* n = logicalOrExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("&&")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_logAnd;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '&&'");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* bitwiseOrExpression(ParserState* ps) {
            ast_node* n = logicalAndExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("|")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_bitOr;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '|'");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* bitwiseXOrExpression(ParserState* ps) {
            ast_node* n = bitwiseOrExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("^")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_xor;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '^'");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* bitwiseAndExpression(ParserState* ps) {
            ast_node* n = bitwiseXOrExpression(ps);
            if (!n) return nullptr;

            if (!ps->isSymbol("&")) return n;
            ps->consume();

            ast_node* o = ps->newNode(nt_expression, n->tok);
            o->op = op_bitAnd;
            o->lvalue = n;
            o->rvalue = expression(ps);
            if (!o->rvalue) {
                ps->error(pec_expected_expr, "Expected expression after '&'");
                ps->freeNode(o);
                return nullptr;
            }
            return o;
        }
        ast_node* equalityExpression(ParserState* ps) {
            ast_node* n = bitwiseAndExpression(ps);
            if (!n) return nullptr;

            if (ps->isSymbol("==")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_compare;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '=='");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol("!=")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_notEq;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '!='");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            return n;
        }
        ast_node* relationalExpression(ParserState* ps) {
            ast_node* n = equalityExpression(ps);
            if (!n) return nullptr;

            if (ps->isSymbol("<")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_lessThan;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '<'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol(">")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_greaterThan;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '>'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol("<=")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_lessThanEq;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '<='");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol(">=")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_greaterThanEq;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '>='");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }
        
            return n;
        }
        ast_node* shiftExpression(ParserState* ps) {
            ast_node* n = relationalExpression(ps);
            if (!n) return nullptr;

            if (ps->isSymbol("<<")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_shLeft;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '<<'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol(">>")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_shRight;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '>>'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            return n;
        }
        ast_node* additiveExpression(ParserState* ps) {
            ast_node* n = shiftExpression(ps);
            if (!n) return nullptr;

            if (ps->isSymbol("+")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_add;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '+'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol("-")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_sub;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '-'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            return n;
        }
        ast_node* multiplicativeExpression(ParserState* ps) {
            ast_node* n = additiveExpression(ps);
            if (!n) return nullptr;

            if (ps->isSymbol("*")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_mul;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '*'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol("/")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_div;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '/'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            if (ps->isSymbol("%")) {
                ps->consume();

                ast_node* o = ps->newNode(nt_expression, n->tok);
                o->op = op_mod;
                o->lvalue = n;
                o->rvalue = expression(ps);
                if (!o->rvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '%'");
                    ps->freeNode(o);
                    return nullptr;
                }
                return o;
            }

            return n;
        }
        ast_node* notExpression(ParserState* ps) {
            if (ps->isSymbol("!")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_not;
                n->lvalue = expression(ps);

                if (!n->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression or literal after '!'");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            }

            return multiplicativeExpression(ps);
        }
        ast_node* invertExpression(ParserState* ps) {
            if (ps->isSymbol("~")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_bitInv;
                n->lvalue = expression(ps);

                if (!n->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression or literal after '~'");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            }

            return notExpression(ps);
        }
        ast_node* negateExpression(ParserState* ps) {
            if (ps->isSymbol("-")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_negate;
                n->lvalue = expression(ps);

                if (!n->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression or literal after '-'");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            }

            return invertExpression(ps);
        }
        ast_node* prefixDecExpression(ParserState* ps) {
            if (ps->isSymbol("--")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_preDec;
                n->lvalue = expression(ps);

                if (!n->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '--'");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            }

            return negateExpression(ps);
        }
        ast_node* prefixIncExpression(ParserState* ps) {
            if (ps->isSymbol("++")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_preInc;
                n->lvalue = expression(ps);

                if (!n->lvalue) {
                    ps->error(pec_expected_expr, "Expected expression after '++'");
                    ps->freeNode(n);
                    return nullptr;
                }

                return n;
            }

            return prefixDecExpression(ps);
        }
        ast_node* postfixDecExpression(ParserState* ps) {
            ast_node* n = prefixIncExpression(ps);

            if (ps->isSymbol("--")) {
                ps->consume();
                ast_node* i = ps->newNode(nt_expression, n->tok);
                i->op = op_postDec;
                i->lvalue = n;
                return i;
            }

            return n;
        }
        ast_node* postfixIncExpression(ParserState* ps) {
            ast_node* n = postfixDecExpression(ps);

            if (ps->isSymbol("++")) {
                ps->consume();
                ast_node* i = ps->newNode(nt_expression, n->tok);
                i->op = op_postInc;
                i->lvalue = n;
                return i;
            }

            return n;
        }
        ast_node* callExpression(ParserState* ps) {
            ast_node* n = postfixIncExpression(ps);
            if (!n) return nullptr;
            
            if (ps->typeIs(tt_open_parenth)) {
                ast_node* c = ps->newNode(nt_expression, n->tok);
                c->op = op_call;
                c->lvalue = n;
                c->parameters = arguments(ps);
                return c;
            }

            return n;
        }
        ast_node* newExpression(ParserState* ps) {
            if (ps->isKeyword("new")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_expression, &ps->getPrev());
                n->op = op_new;
                n->data_type = typeSpecifier(ps);

                if (!n->data_type) {
                    ps->error(pec_expected_type_name, "Expected type name after 'new'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (ps->typeIs(tt_open_parenth)) {
                    n->parameters = arguments(ps);
                }

                return n;
            }

            return callExpression(ps);
        }
        ast_node* memberExpression(ParserState* ps) {
            ast_node* n = newExpression(ps);
            if (!n) return nullptr;

            if (!ps->typeIs(tt_dot)) return n;
            ps->consume();

            ast_node* i = ps->newNode(nt_expression, n->tok);
            i->op = op_member;
            i->lvalue = n;
            
            const token& t = ps->get();
            if (t.tp != tt_identifier) {
                ps->error(pec_expected_identifier, "Expected identifier after '.'");
                ps->freeNode(i);
                return nullptr;
            }
            i->value.s = t.text.c_str();
            i->str_len = t.text.size();

            return i;
        }
        ast_node* indexExpression(ParserState* ps) {
            ast_node* n = memberExpression(ps);
            if (!n) return nullptr;

            if (!ps->typeIs(tt_open_bracket)) return n;
            ps->consume();

            ast_node* i = ps->newNode(nt_expression, n->tok);
            i->op = op_index;
            i->lvalue = n;
            i->rvalue = expressionSequence(ps);
            if (!i->rvalue) {
                ps->error(pec_expected_expr, "Expected expression");
                ps->freeNode(i);
                return nullptr;
            }

            if (!ps->typeIs(tt_close_bracket)) {
                ps->error(pec_expected_closing_bracket, "Expected ']'");
                // attempt to continue
            } else ps->consume();

            return i;
        }
        ast_node* expression(ParserState* ps) {
            ast_node* n = anonymousFunction(ps);
            if (!n) return indexExpression(ps);
            return n;
        }
        
        ast_node* variableDecl(ParserState* ps) {
            ast_node* _assignable = one_of(ps, { typedAssignable, assignable, objectDecompositor });
            if (!_assignable) return nullptr;

            ast_node* decl = ps->newNode(nt_variable, _assignable->tok);
            decl->body = _assignable;
            if (ps->isSymbol("=")) {
                ps->consume();
                decl->initializer = expression(ps);
                if (!decl->initializer) {
                    ps->error(pec_expected_expr, "Expected expression for variable initializer");
                }
            }

            return decl;
        }
        ast_node* variableDeclList(ParserState* ps) {
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
            f->tok = tok;

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
        ast_node* variableStatement(ParserState* ps) {
            return all_of(ps, { variableDeclList, eosRequired });
        }
        ast_node* emptyStatement(ParserState* ps) {
            return eos(ps);
        }
        ast_node* expressionSequence(ParserState* ps) {
            ast_node* f = expression(ps);
            if (!f) return nullptr;
            
            ast_node* n = f;
            while(true) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = expression(ps);
                n = n->next;

                if (!n) {
                    ps->error(pec_expected_expr, "Expected expression after ','");
                    ps->freeNode(f);
                    return nullptr;
                }
            }

            return f;
        }
        ast_node* expressionSequenceGroup(ParserState* ps) {
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
        ast_node* expressionStatement(ParserState* ps) {
            return all_of(ps, { expressionSequence, eosRequired });
        }
        ast_node* ifStatement(ParserState* ps) {
            if (!ps->isKeyword("if")) return nullptr;
            ps->consume();
            ast_node* n = ps->newNode(nt_if, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pec_expected_expr_group, "Expected '(<expression>)' after 'if'");
                ps->freeNode(n);
                return nullptr;
            }

            n->body = statement(ps);
            if (!n->body) {
                ps->error(pec_expected_statement, "Expected block or statement after 'if (<expression>)'");
                ps->freeNode(n);
                return nullptr;
            }
            
            if (ps->isKeyword("else")) {
                ps->consume();
                n->else_body = statement(ps);
                if (!n->else_body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'else'");
                    ps->freeNode(n);
                    return nullptr;
                }
            }

            return n;
        }
        ast_node* continueStatement(ParserState* ps) {
            if (!ps->isKeyword("continue")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after 'continue'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ast_node* breakStatement(ParserState* ps) {
            if (!ps->isKeyword("break")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after 'break'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ast_node* iterationStatement(ParserState* ps) {
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
                    ps->error(pec_expected_expr_group, "Expected '(<expression>)' after 'do ... while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected ';' after 'do ... while (<expression>)'");
                    // attempt to continue
                } else ps->consume();
                
                n->flags.defer_cond = 1;
                return n;
            } else if (ps->isKeyword("while")) {
                ps->consume();
                ast_node* n = ps->newNode(nt_loop, &ps->getPrev());
                n->cond = expressionSequenceGroup(ps);
                if (!n->cond) {
                    ps->error(pec_expected_expr_group, "Expected '(<expression>)' after 'while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pec_expected_statement, "Expected block or statement after 'while (<expression>)'");
                    ps->freeNode(n);
                    return nullptr;
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

                return n;
            }

            return nullptr;
        }
        ast_node* returnStatement(ParserState* ps) {
            if (!ps->isKeyword("return")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_return, &ps->getPrev());
            n->body = expression(ps);

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pec_expected_eos, "Expected ';' after return statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ast_node* switchCase(ParserState* ps) {
            if (!ps->isKeyword("case")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_switch_case, &ps->getPrev());
            n->cond = expressionSequence(ps);
            if (!n->cond) {
                ps->error(pec_expected_expr_group, "Expected '(<expression>)' after 'case'");
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
        ast_node* switchStatement(ParserState* ps) {
            if (!ps->isKeyword("switch")) return nullptr;
            ps->consume();

            ast_node* n = ps->newNode(nt_return, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pec_expected_expr_group, "Expected '(<expression>)' after 'switch'");
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
        ast_node* throwStatement(ParserState* ps) {
            if (!ps->isKeyword("throw")) return nullptr;
            const token* t = &ps->get();
            ps->consume();
            ast_node* expr = expression(ps);
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
        ast_node* catchBlock(ParserState* ps) {
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
        ast_node* tryStatement(ParserState* ps) {
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
        ast_node* functionDecl(ParserState* ps) {
            if (!ps->isKeyword("function")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* n = identifier(ps);
            if (!n) {
                ps->error(pec_expected_identifier, "Expected identifier after 'function'");
                return nullptr;
            }

            n->tp = nt_function;
            n->tok = t;
            n->parameters = parameterList(ps);
            if (!n->parameters) {
                ps->error(pec_expected_parameter_list, "Expected function parameter list");
                ps->freeNode(n);
                return nullptr;
            }

            return n;
        }
        ast_node* functionDef(ParserState* ps) {
            ast_node* n = functionDecl(ps);
            if (!n) return nullptr;

            n->body = block(ps);

            if (!n->body) {
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pec_expected_eos, "Expected either ';' or a function body");
                    // attempt to continue
                } else ps->consume();
            }

            return n;
        }
        ast_node* classDef(ParserState* ps) { return nullptr; }
        ast_node* typeDef(ParserState* ps) {
            if (!ps->isKeyword("type")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ast_node* n = identifier(ps);

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
            n->tok = t;
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
        ast_node* symbolImport(ParserState* ps) {
            ast_node* n = one_of(ps, {
                typedIdentifier,
                identifier
            });
            if (!n) return nullptr;
            n->tp = nt_import_symbol;
            return n;
        }
        ast_node* importList(ParserState* ps) {
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
        ast_node* importModule(ParserState* ps) {
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
        ast_node* declaration(ParserState* ps) {
            return one_of(ps, {
                variableStatement,
                classDef,
                typeDef,
                functionDef
            });
        }
        ast_node* importStatement(ParserState* ps) {
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
        ast_node* exportStatement(ParserState* ps) {
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
        ast_node* statement(ParserState* ps) {
            ast_node* n = one_of(ps, {
                block,
                variableStatement,
                importStatement,
                exportStatement,
                emptyStatement,
                classDef,
                typeDef,
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
        ast_node* program(ParserState* ps) {
            ast_node* statements = array_of(ps, statement);

            if (!ps->typeIs(tt_eof)) {
                ps->error(pec_expected_eof, "Expected end of file");
                if (statements) ps->freeNode(statements);
                return nullptr;
            }
            ps->consume();

            if (!statements) return nullptr;
            
            ast_node* root = ps->newNode(nt_root, statements->tok);
            root->body = statements;
            return root;
        }
    };
};