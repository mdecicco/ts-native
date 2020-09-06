#include <lexer/token.h>

namespace gjs {
    namespace lex {
        token::token() : src(source_ref()) {
        }

        token::token(const token& t) {
            text = t.text;
            type = t.type;
            src = t.src;
        }

        token::token(const std::string& _text, token_type _type, const source_ref& _src) {
            text = _text;
            type = _type;
            src = _src;
        }

        bool token::operator == (const std::string& rhs) const {
            return text == rhs;
        }

        bool token::operator == (token_type rhs) const {
            return type == rhs;
        }
    };
};