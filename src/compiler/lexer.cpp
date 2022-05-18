#include <gs/compiler/lexer.h>
#include <gs/utils/ProgramSource.h>
#include <utils/Array.hpp>

namespace gs {
    namespace compiler {
        Lexer::Lexer(ProgramSource* src) : m_curSrc(src, 0, 0) {
            m_source = src;
        }

        Lexer::~Lexer() {

        }

        utils::Array<token> Lexer::tokenize() {
            reset();
            if (!m_curSrc.isValid()) return {};

            utils::Array<token> out;
            
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

                if (isalpha(*begin)) {
                    tp = tt_keyword;
                    while (!(at_end = !m_curSrc++)) {
                        end++;
                        char c = *m_curSrc;
                        if (!isalnum(c) && c != '_') break;
                    }

                    const char* tmp = begin;
                    char n = *tmp;
                    tmp++;
                    switch (n) {
                        case 'a': {
                            // as
                            if (*tmp++ != 's') tp = tt_identifier;
                            break;
                        }
                        case 'b': {
                            // break
                            if (
                                *tmp++ != 'r' ||
                                *tmp++ != 'e' ||
                                *tmp++ != 'a' ||
                                *tmp++ != 'k'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'c': {
                            // class, const, continue
                            n = *tmp++;
                            if (n == 'l') {
                                if (
                                    *tmp++ != 'a' ||
                                    *tmp++ != 's' ||
                                    *tmp++ != 's'
                                ) {
                                    tp = tt_identifier;
                                }
                            } else if (n == 'o') {
                                n = *tmp++;
                                if (n != 'n') tp = tt_identifier;
                                else {
                                    n = *tmp++;
                                    if (n == 's') {
                                        if (*tmp++ != 't') tp = tt_identifier;
                                    } else if (n == 't') {
                                        if (
                                            *tmp++ != 'i' ||
                                            *tmp++ != 'n' ||
                                            *tmp++ != 'u' ||
                                            *tmp++ != 'e'
                                        ) {
                                            tp = tt_identifier;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case 'd': {
                            // do
                            if (*tmp++ != 'o') {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'e': {
                            // else, enum, export, extends
                            n = *tmp++;
                            if (n == 'l') {
                                if (
                                    *tmp++ != 's' ||
                                    *tmp++ != 'e'
                                ) {
                                    tp = tt_identifier;
                                }
                            } else if (n == 'n') {
                                if (
                                    *tmp++ != 'u' ||
                                    *tmp++ != 'm'
                                ) {
                                    tp = tt_identifier;
                                }
                            } else if (n == 'x') {
                                n = *tmp++;
                                if (n == 'p') {
                                    if (
                                        *tmp++ != 'o' ||
                                        *tmp++ != 'r' ||
                                        *tmp++ != 't'
                                    ) {
                                        tp = tt_identifier;
                                    }
                                } else if (n == 't') {
                                    if (
                                        *tmp++ != 'e' ||
                                        *tmp++ != 'n' ||
                                        *tmp++ != 'd' ||
                                        *tmp++ != 's'
                                    ) {
                                        tp = tt_identifier;
                                    }
                                }
                            }
                            break;
                        }
                        case 'f': {
                            // for, from
                            n = *tmp++;
                            if (n == 'o') {
                                if (*tmp++ != 'r') {
                                    tp = tt_identifier;
                                }
                            } else if (n == 'r') {
                                if (
                                    *tmp++ != 'o' ||
                                    *tmp++ != 'm'
                                ) {
                                    tp = tt_identifier;
                                }
                            }
                            break;
                        }
                        case 'g': {
                            // get
                            if (
                                *tmp++ != 'e' ||
                                *tmp++ != 't'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'i': {
                            // if, import
                            n = *tmp++;
                            if (n == 'f') {
                                // tp is already tt_keyword
                            } else if (n == 'm') {
                                if (
                                    *tmp++ != 'p' ||
                                    *tmp++ != 'o' ||
                                    *tmp++ != 'r' ||
                                    *tmp++ != 't'
                                ) {
                                    tp = tt_identifier;
                                }
                            }
                            break;
                        }
                        case 'n': {
                            // null
                            if (
                                *tmp++ != 'u' ||
                                *tmp++ != 'l' ||
                                *tmp++ != 'l'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'o': {
                            // operator
                            if (*tmp++ != 'p' ||
                                *tmp++ != 'e' ||
                                *tmp++ != 'r' ||
                                *tmp++ != 'a' ||
                                *tmp++ != 't' ||
                                *tmp++ != 'o' ||
                                *tmp++ != 'r' ||
                                isalnum(*tmp) || *tmp == '_'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'p': {
                            // private, public
                            n = *tmp++;
                            if (n == 'u') {
                                if (
                                    *tmp++ != 'b' ||
                                    *tmp++ != 'l' ||
                                    *tmp++ != 'i' ||
                                    *tmp++ != 'c'
                                ) {
                                    tp = tt_identifier;
                                }
                            } else if (n == 'r') {
                                if (
                                    *tmp++ != 'i' ||
                                    *tmp++ != 'v' ||
                                    *tmp++ != 'a' ||
                                    *tmp++ != 't' ||
                                    *tmp++ != 'e'
                                ) {
                                    tp = tt_identifier;
                                }
                            }
                            break;
                        }
                        case 's': {
                            // set, static
                            n = *tmp++;
                            if (n == 'e') {
                                if (*tmp++ != 't') tp = tt_identifier;
                            } else if (n == 't') {
                                if (
                                    *tmp++ != 'a' ||
                                    *tmp++ != 't' ||
                                    *tmp++ != 'i' ||
                                    *tmp++ != 'c'
                                ) {
                                    tp = tt_identifier;
                                }
                            }
                            break;
                        }
                        case 't': {
                            // type
                            if (
                                *tmp++ != 'y' ||
                                *tmp++ != 'p' ||
                                *tmp++ != 'e'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        case 'w': {
                            // while
                            if (
                                *tmp++ != 'h' ||
                                *tmp++ != 'i' ||
                                *tmp++ != 'l' ||
                                *tmp++ != 'e'
                            ) {
                                tp = tt_identifier;
                            }
                            break;
                        }
                        default: tp = tt_identifier;
                    }

                    if (tp == tt_keyword && (isalnum(*tmp) || *tmp == '_')) tp = tt_identifier;
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
                        utils::String::View(begin, end - begin),
                        tokBegin
                    });

                    // check for number postfix
                    begin = m_curSrc.getPointer();
                    char n = ::tolower(*begin);

                    if (n == 'f' || n == 'b' || n == 's') {
                        out.push({
                            tt_number_suffix,
                            utils::String::View(m_curSrc.getPointer(), 1),
                            m_curSrc
                        });
                        
                        at_end = !m_curSrc++;
                    } else if (n == 'u') {
                        n = ::tolower(*(begin + 1));
                        if (n == 's' || n == 'b') {
                            out.push({
                                tt_number_suffix,
                                utils::String::View(m_curSrc.getPointer(), 2),
                                m_curSrc
                            });
                            
                            at_end = !m_curSrc++;
                            at_end = !m_curSrc++;
                        } else if (n == 'l') {
                            n = ::tolower(*(begin + 2));

                            out.push({
                                tt_number_suffix,
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

                    while (!at_end) {
                        if (*end == '\\') {
                            end++;
                            at_end = !m_curSrc++;
                            if (at_end) {
                                throw SourceException("Encountered unexpected end of input while scanning string literal", tokBegin);
                            }

                            switch (*end) {
                                case 'n' : { str += '\n'; break; }
                                case 'r' : { str += '\r'; break; }
                                case 't' : { str += '\t'; break; }
                                case '\\': { str += '\\'; break; }
                                case '\'': { str += '\''; break; }
                                case '"' : { str += '"' ; break; }
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
                                str,
                                tokBegin
                            });

                            break;
                        }
                        
                        str += *end;
                        end++;
                        at_end = !m_curSrc++;
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
                        }
                        case '+' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
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
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
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
                            }
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
                            }
                        }
                        case '=' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                        }
                        case '>' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                        }
                        case '?' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;
                            tp = tt_symbol;
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
                        }
                        case '}' : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_close_brace;
                            break;
                        }
                        case '~' : {
                            end++;
                            tmp++;
                            at_end = !m_curSrc++;

                            tp = tt_symbol;
                            if (*tmp == '=') {
                                end++;
                                at_end = !m_curSrc++;
                            }
                        }
                        default  : {
                            end++;
                            at_end = !m_curSrc++;
                            tp = tt_unknown;
                            break;
                        }
                    }
                }
            
                if (begin != end) {
                    out.push({
                        tp,
                        utils::String::View(begin, end - begin),
                        tokBegin
                    });
                }
            }

            return out;
        }

        void Lexer::reset() {
            m_curSrc = SourceLocation(m_source, 0, 0);
        }
    };
};