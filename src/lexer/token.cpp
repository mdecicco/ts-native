#include <lexer/token.h>

namespace gjs {
    namespace lex {
        token::token() {
        }
        token::token(const std::string& _text, token_type _type, const source_ref& _src) {
            text = _text;
            type = _type;
            src = _src;
        }

        bool token::operator == (const std::string& rhs) const {
            return text == rhs;
        }
    };
};