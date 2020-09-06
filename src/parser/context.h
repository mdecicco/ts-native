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
            std::vector<std::string> type_names;
            u32 cur_token;

            context(vm_context* ctx);

            bool is_typename(const std::string& text) const;

            const lex::token& current() const;
            void consume();
            bool at_end() const;

            // current() == any of the provided strings
            bool match(const std::vector<std::string>& any);

            // because apparently sometimes std::string is convertible to lex::token_type...
            bool match_s(const std::vector<std::string>& any);

            // current() == any of the provided token types
            bool match(const std::vector<lex::token_type>& any);

            // next sizeof(sequence) tokens match the strings in the sequence
            bool pattern(const std::vector<std::string>& sequence);

            // because apparently sometimes std::string is convertible to lex::token_type...
            bool pattern_s(const std::vector<std::string>& any);

            // next sizeof(sequence) tokens match the token types in the sequence
            bool pattern(const std::vector<lex::token_type>& sequence);
        };
    };
};