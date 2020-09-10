#pragma once
#include <common/types.h>
#include <common/source_ref.h>
#include <string>

namespace gjs {
    namespace lex {
        enum class token_type {
            invalid_token,
            any, // for matching with any token in a pattern
            identifier,
            keyword,
            operation,
            open_parenth,
            close_parenth,
            open_block,
            close_block,
            open_bracket,
            close_bracket,
            integer_literal,
            decimal_literal,
            string_literal,
            line_comment,
            block_comment,
            semicolon,
            question_mark,
            member_accessor,
            comma,
            colon
        };

        struct token {
            std::string text;
            token_type type;
            source_ref src;

            token();
            token(const token& t);
            token(const std::string& text, token_type type, const source_ref& src);

            operator source_ref() const;
            operator std::string() const;
            bool operator == (const std::string& rhs) const;
            bool operator == (token_type rhs) const;
        };
    };
};