#include <lexer/token.h>
#include <stack>
#include <vector>

namespace gjs {
    class vm_context;

    namespace parse {
        struct ast;

        struct context {
            vm_context* env;
            ast* root;
            std::stack<ast*> path;
            std::vector<lex::token> tokens;
            u32 cur_token;

            context();
        };
    };
};