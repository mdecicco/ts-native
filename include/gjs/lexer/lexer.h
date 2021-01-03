#include <string>
#include <vector>
#include <gjs/lexer/token.h>

namespace gjs {
    struct log_file;
    namespace lex {
        void tokenize(const std::string& source, const std::string& module, std::vector<token>& out, log_file& out_file);
    };
};