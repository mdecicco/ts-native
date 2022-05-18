#pragma once
#include <gs/common/types.h>
#include <gs/utils/SourceLocation.h>
#include <utils/String.h>

#include <exception>

namespace gs {
    class ProgramSource;
    class SourceLocation;

    namespace compiler {
        enum token_type {
            tt_unknown,
            tt_identifier,
            // if, else, do, while, for, break, continue, type, enum,
            // class, extends, public, private, import, export, from,
            // as, operator, static, const, get, set, null
            tt_keyword,

            // + , - , * , / , ~ , ! , % , ^ , < , > , = , & , && , | , || ,
            // +=, -=, *=, /=, ~=, !=, %=, ^=, <=, >=, ==, &=, &&=, |=, ||=,
            // ?
            tt_symbol,
            tt_number,

            // [f, b, ub, s, us, ul, ll, ull] (case insensitive)
            tt_number_suffix,
            tt_right_arrow,                     // =>
            tt_string,                          // ['...', "..."]
            tt_template_string,                 // `...`
            tt_dot,
            tt_comma,
            tt_colon,
            tt_semicolon,
            tt_open_parenth,
            tt_close_parenth,
            tt_open_brace,
            tt_close_brace,
            tt_open_bracket,
            tt_close_bracket,
            tt_eof
        };

        struct token {
            token_type tp;

            // View of ProgramSource::m_rawCode, read only, unless tp == tt_string | tt_template_string
            utils::String text;
            SourceLocation src;
        };

        class Lexer {
            public:
                Lexer(ProgramSource* src);
                ~Lexer();

                utils::Array<token> tokenize();

            private:
                void reset();

                ProgramSource* m_source;
                SourceLocation m_curSrc;
        };
    };
};