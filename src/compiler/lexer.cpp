#include <tsn/compiler/Lexer.h>
#include <tsn/compiler/Lexer.hpp>
#include <tsn/utils/ModuleSource.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace compiler {
        const char* allKeywords[] = {
            "as",
            "break",
            "case",
            "catch",
            "class",
            "const",
            "continue",
            "default",
            "delete",
            "do",
            "else",
            "enum",
            "export",
            "extends",
            "false",
            "for",
            "from",
            "function",
            "get",
            "if",
            "import",
            "let",
            "new",
            "null",
            "operator",
            "private",
            "public",
            "return",
            "set",
            "sizeof",
            "static",
            "switch",
            "this",
            "throw",
            "true",
            "try",
            "type",
            "typeinfo",
            "while",
        };
        constexpr u32 keywordCount = sizeof(allKeywords) / sizeof(const char*);
        
        utils::String token::str() const {
            return utils::String(const_cast<char*>(text.c_str()), text.size());
        }

        Lexer::Lexer(ModuleSource* src) : m_curSrc(src, 0, 0) {
            m_source = src;
        }

        Lexer::~Lexer() {

        }

        utils::Array<token> Lexer::tokenize() {
            reset();
            utils::Array<token> out;

            if (!m_curSrc.isValid()) {
                out.push({
                    tt_eof,
                    0,
                    utils::String::View(m_curSrc.getPointer(), 0),
                    m_curSrc
                });
                return out;
            }
            
            bool at_end = false;
            while (!at_end) {
                const char* begin = m_curSrc.getPointer();
                while (isspace(*begin) && !at_end) {
                    at_end = !m_curSrc++;
                    if (!at_end) begin++;
                }
                if (at_end) break;

                SourceLocation tokBegin = m_curSrc;
                const char* end = begin;

                token_type tp = tt_unknown;

                if (isalpha(*begin) || *begin == '_') {
                    tp = tt_keyword;
                    while (end++ && !(at_end = !m_curSrc++)) {
                        if (!isalnum(*end) && *end != '_') break;
                    }

                    const char* tmp = begin;
                    i32 matchIdx = stringMatchesOne<keywordCount>(allKeywords, begin, &tmp);

                    if (matchIdx == -1) tp = tt_identifier;
                    else if (isalnum(*tmp) || *tmp == '_') tp = tt_identifier;
                } else if (isdigit(*begin) || (*begin == '-' && isdigit(*(begin + 1)))) {
                    if (*begin == '-') {
                        end++;
                        at_end = !m_curSrc++;
                    }
                    bool decimal = false;

                    while (!at_end) {
                        if (*end == '.') {
                            if (decimal) break;
                            decimal = true;
                            
                            if (*(end + 1) == '.') {
                                throw SourceException(
                                    "Encountered malformed number literal. If the intention "
                                    "was to use the member accessor operator ('.') on a floating "
                                    "point literal, add a zero after the decimal point and before "
                                    "the member accessor operator",
                                    tokBegin
                                );
                            }
                        } else if (!isdigit(*end)) {
                            break;
                        }
                        
                        at_end = !m_curSrc++;
                        end++;
                    }

                    out.push({
                        tt_number,
                        u16(end - begin),
                        utils::String::View(begin, u32(end - begin)),
                        tokBegin
                    });

                    // check for number postfix
                    begin = m_curSrc.getPointer();
                    char n = ::tolower(*begin);

                    if (n == 'f' || n == 'b' || n == 's') {
                        out.push({
                            tt_number_suffix,
                            1,
                            utils::String::View(m_curSrc.getPointer(), 1),
                            m_curSrc
                        });
                        
                        at_end = !m_curSrc++;
                    } else if (n == 'l') {
                        if (::tolower(*(begin + 1) == 'l')) {
                            out.push({
                                tt_number_suffix,
                                2,
                                utils::String::View(m_curSrc.getPointer(), 2),
                                m_curSrc
                            });

                            at_end = !m_curSrc++;
                            at_end = !m_curSrc++;
                        }
                    } else if (n == 'u') {
                        n = ::tolower(*(begin + 1));
                        if (n == 's' || n == 'b') {
                            out.push({
                                tt_number_suffix,
                                2,
                                utils::String::View(m_curSrc.getPointer(), 2),
                                m_curSrc
                            });
                            
                            at_end = !m_curSrc++;
                            at_end = !m_curSrc++;
                        } else if (n == 'l') {
                            n = ::tolower(*(begin + 2));

                            out.push({
                                tt_number_suffix,
                                u16(n == 'l' ? 3 : 2),
                                utils::String::View(m_curSrc.getPointer(), n == 'l' ? 3 : 2),
                                m_curSrc
                            });
                            
                            at_end = !m_curSrc++;
                            at_end = !m_curSrc++;
                            if (n == 'l') at_end = !m_curSrc++;
                        }
                    }

                    continue;
                } else if (*begin == '\'' || *begin == '"' || *begin == '`') {
                    utils::String str;
                    char q = *begin;
                    end++;
                    at_end = !m_curSrc++;
                    bool terminated = false;

                    while (!at_end) {
                        if (*end == '\\') {
                            end++;
                            at_end = !m_curSrc++;
                            if (at_end) break;

                            switch (*end) {
                                case 'n' : { str += '\n'; break; }
                                case 'r' : { str += '\r'; break; }
                                case 't' : { str += '\t'; break; }
                                case '\\': { str += '\\'; break; }
                                case '\'': { str += '\''; break; }
                                case '"' : { str += '"' ; break; }
                                case '$' : { str += '$' ; break; }
                                case '`' : { str += '`' ; break; }
                                default  : {
                                    // todo: Unknown escape sequence warning
                                    break;
                                }
                            }

                            end++;
                            at_end = !m_curSrc++;
                            continue;
                        }

                        if (*end == q) {
                            at_end = !m_curSrc++;
                            out.push({
                                q == '`' ? tt_template_string : tt_string,
                                u16(str.size()),
                                str,
                                tokBegin
                            });
                            terminated = true;
                            break;
                        }
                        
                        str += *end;
                        end++;
                        at_end = !m_curSrc++;
                    }

                    if (!terminated) {
                        throw SourceException("Encountered unexpected end of input while scanning string literal", tokBegin);
                    }
                    continue;
                } else {
                    const char* tmp = begin;
                    char n = *tmp;
                    switch (n) {
                        case '!' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '%' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '&' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            n = *tmp++;

                            if (n == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (n == '&') {
                                end++;
                                at_end = !m_curSrc++;

                                if (*tmp == '=') {
                                    end++;
                                    at_end = !m_curSrc++;
                                }
                            }
                            break;
                        }
                        case '(' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_open_parenth;
                            break;
                        }
                        case ')' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_close_parenth;
                            break;
                        }
                        case '*' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '+' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=' || *tmp == '+') {
                                end++;
                                at_end = !m_curSrc++;
                            } 
                            break;
                        }
                        case ',' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_comma;
                            break;
                        }
                        case '-' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=' || *tmp == '-') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '.' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_dot;
                            break;
                        }
                        case '/' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (*tmp == '/') {
                                end++;
                                at_end = !m_curSrc++;

                                tp = tt_comment;
                                while (!at_end && (*end) != '\n' && (*end) != '\r') {
                                    end++;
                                    at_end = !m_curSrc++;
                                }
                            } else if (*tmp == '*') {
                                end++;
                                at_end = !m_curSrc++;

                                tp = tt_comment;
                                while (!at_end) {
                                    if ((*(end - 1)) == '*' && (*end) == '/') {
                                        end++;
                                        at_end = !m_curSrc++;
                                        break;
                                    }
                                    end++;
                                    at_end = !m_curSrc++;
                                }
                            }
                            break;
                        }
                        case ':' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_colon;
                            break;
                        }
                        case ';' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_semicolon;
                            break;
                        }
                        case '<' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (*tmp == '<') {
                                end++;
                                at_end = !m_curSrc++;
                                tmp++;

                                if (*tmp == '=') {
                                    end++;
                                    at_end = !m_curSrc++;
                                    tmp++;
                                }
                            }
                            break;
                        }
                        case '=' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (*tmp == '>') {
                                tp = tt_forward;
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '>' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (*tmp == '>') {
                                end++;
                                at_end = !m_curSrc++;
                                tmp++;

                                if (*tmp == '=') {
                                    end++;
                                    at_end = !m_curSrc++;
                                    tmp++;
                                }
                            }
                            break;
                        }
                        case '?' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;
                            tp = tt_symbol;
                            break;
                        }
                        case '[' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_open_bracket;
                            break;
                        }
                        case ']' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_close_bracket;
                            break;
                        }
                        case '^' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                            break;
                        }
                        case '{' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_open_brace;
                            break;
                        }
                        case '|' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            n = *tmp++;

                            if (n == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            } else if (n == '|') {
                                end++;
                                at_end = !m_curSrc++;

                                if (*tmp == '=') {
                                    end++;
                                    at_end = !m_curSrc++;
                                }
                            }
                            break;
                        }
                        case '}' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_close_brace;
                            break;
                        }
                        case '~' : {
                            tp = tt_symbol;
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;
                            break;
                        }
                        default  : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_unknown;
                            break;
                        }
                    }
                }
            
                if (begin != end && tp != tt_comment) {
                    u32 len = u32(end - begin);
                    out.push({
                        tp,
                        u16(len),
                        utils::String::View(begin, len),
                        tokBegin
                    });
                }
            }

            out.push({
                tt_eof,
                0,
                utils::String::View(m_curSrc.getPointer(), 0),
                m_curSrc
            });

            return out;
        }

        void Lexer::reset() {
            m_curSrc = SourceLocation(m_source, 0, 0);
        }
    };
};