#pragma once
#include <gjs/common/types.h>
#include <gjs/common/errors.h>
#include <gjs/util/util.h>
#include <string>
#include <stack>
#include <vector>

namespace gjs {
    class tokenizer {
        public:
            tokenizer(const std::string& input);
            ~tokenizer();

            struct token {
                u32 line = 0;
                u32 col = 0;
                std::string text = "";
                std::string module = "";
                bool is_empty_string_literal = false;

                operator bool() const { return text.length() != 0 || is_empty_string_literal; }
                inline const bool operator == (const token& rhs) const { return text == rhs.text && is_empty_string_literal == rhs.is_empty_string_literal; }
                inline const bool operator == (const std::string& rhs) const { return text == rhs; }
            };

            void specify_keyword(const std::string& keyword);
            void specify_operator(const std::string& op);
            bool is_keyword(const std::string& thing);
            bool is_operator(const std::string& op);
            bool is_number(const std::string& thing);
            bool is_identifier(const std::string& thing);
            inline std::string line_str() const { return lines[m_line]; }
            inline u32 line() const { return m_line; }
            inline u32 col() const { return m_col; }
            inline const bool at_end(bool check_whitespace = true) {
                if (check_whitespace) whitespace();
                return m_idx == m_input.length();
            }

            void backup_state();
            void commit_state();
            void restore_state();

            bool whitespace();
            bool newline();
            std::string thing();
            token any_token();
            token semicolon(bool expected = true);
            token keyword(bool expected = true, const std::string& kw = "");
            token operat0r(bool expected = true, const std::string& op = "");
            token identifier(bool expected = true, const std::string& identifier = "");
            token character(char c, bool expected = true);
            token expression(bool expected = true);
            token string_constant(bool expected = true, bool strip_quotes = false, bool allow_empty = true);
            token number_constant(bool expected = true);
            token line_comment();
            token block_comment();

            std::vector<std::string> lines;
            std::string module;

        protected:
            std::vector<std::string> m_keywords;
            std::vector<std::string> m_operators;
            std::string m_input;
            struct stored_state {
                u64 idx;
                u32 line;
                u32 col;
            };
            std::stack<stored_state> m_storedState;
            u64 m_idx;
            u32 m_line;
            u32 m_col;
    };
};