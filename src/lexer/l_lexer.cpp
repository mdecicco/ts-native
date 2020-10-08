#include <lexer/token.h>
#include <lexer/lexer.h>
#include <parser/parse_utils.h>

namespace gjs {
    namespace lex {
        void tokenize(const std::string& source, const std::string& file, std::vector<token>& out) {
            tokenizer t(source);
            t.specify_keyword("if");
            t.specify_keyword("else");
            t.specify_keyword("while");
            t.specify_keyword("for");
            t.specify_keyword("do");
            t.specify_keyword("class");
            t.specify_keyword("format");
            t.specify_keyword("return");
            t.specify_keyword("this");
            t.specify_keyword("const");
            t.specify_keyword("static");
            t.specify_keyword("null");
            t.specify_keyword("new");
            t.specify_keyword("delete");
            t.specify_keyword("constructor");
            t.specify_keyword("destructor");
            t.specify_keyword("operator");
            t.specify_keyword("import");
            t.specify_operator("++");
            t.specify_operator("--");
            t.specify_operator("+");
            t.specify_operator("-");
            t.specify_operator("*");
            t.specify_operator("/");
            t.specify_operator("%");
            t.specify_operator("&");
            t.specify_operator("|");
            t.specify_operator("^");
            t.specify_operator("+=");
            t.specify_operator("-=");
            t.specify_operator("*=");
            t.specify_operator("/=");
            t.specify_operator("%=");
            t.specify_operator("&=");
            t.specify_operator("|=");
            t.specify_operator("^=");
            t.specify_operator("&&");
            t.specify_operator("||");
            t.specify_operator(">>");
            t.specify_operator("<<");
            t.specify_operator(">>=");
            t.specify_operator("<<=");
            t.specify_operator("<");
            t.specify_operator("<=");
            t.specify_operator(">");
            t.specify_operator(">=");
            t.specify_operator("!");
            t.specify_operator("=");
            t.specify_operator("==");
            t.specify_operator("[]");

            while (!t.at_end(false)) {
                tokenizer::token tok;
                tok = t.identifier(false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::identifier,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.operat0r(false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::operation,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }
                
                tok = t.keyword(false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::keyword,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.string_constant(false, true);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::string_literal,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.number_constant(false);
                if (tok) {
                    if (tok.text.find_first_of('.') != std::string::npos) {
                        out.push_back({
                            tok.text,
                            token_type::decimal_literal,
                            source_ref(file, t.lines[tok.line], tok.line, tok.col)
                        });
                    } else {
                        out.push_back({
                            tok.text,
                            token_type::integer_literal,
                            source_ref(file, t.lines[tok.line], tok.line, tok.col)
                        });
                    }
                    continue;
                }
                
                tok = t.line_comment();
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::operation,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.block_comment();
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::operation,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character(';', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::semicolon,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('(', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::open_parenth,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character(')', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::close_parenth,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('[', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::open_bracket,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character(']', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::close_bracket,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('{', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::open_block,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('}', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::close_block,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('?', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::question_mark,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character(':', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::colon,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character('.', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::member_accessor,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.character(',', false);
                if (tok) {
                    out.push_back({
                        tok.text,
                        token_type::comma,
                        source_ref(file, t.lines[tok.line], tok.line, tok.col)
                    });
                    continue;
                }

                tok = t.any_token();
                if (tok) throw parse_exception(format("Unknown token '%s'", tok.text.c_str()), t, tok);
            }
        }
    };
};