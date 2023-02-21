#pragma once
#include <tsn/common/types.h>
#include <tsn/utils/SourceLocation.h>
#include <utils/String.h>

#include <exception>

namespace tsn {
    class ModuleSource;
    class SourceLocation;

    namespace compiler {
        enum token_type {
            tt_unknown,
            tt_identifier,
            // if, else, do, while, for, break, continue,
            // type, enum, class, extends, public, private,
            // import, export, from, as, operator, static,
            // const, get, set, null, return, switch, case,
            // default, true, false, this, function, let
            // new, sizeof, delete
            tt_keyword,

            // + , - , *  , / , ~  , ! , % , ^ , < , > , = , & , && , | , || ,
            // +=, -=, *= , /=,    , !=, %=, ^=, <=, >=, ==, &=, &&=, |=, ||=,
            // ? , <<, <<=, >>, >>=
            tt_symbol,
            tt_number,

            // [f, b, ub, s, us, ul, ll, ull] (case insensitive)
            tt_number_suffix,
            tt_forward,         // =>
            tt_string,          // ['...', "..."]
            tt_template_string, // `...`
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
            tt_comment,
            tt_eof
        };

        struct token {
            token_type tp;
            u16 len;

            // View of ModuleSource::m_rawCode, read only, unless tp == tt_string | tt_template_string
            utils::String text;
            SourceLocation src;

            utils::String str() const;
        };

        class Lexer {
            public:
                Lexer(ModuleSource* src);
                ~Lexer();

                utils::Array<token> tokenize();

            private:
                void reset();

                ModuleSource* m_source;
                SourceLocation m_curSrc;
        };
    };
};