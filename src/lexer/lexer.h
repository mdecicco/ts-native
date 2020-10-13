#include <string>
#include <vector>
#include <lexer/token.h>

namespace gjs {
    namespace lex {
        void tokenize(const std::string& source, const std::string& module, std::vector<token>& out);
    };
};