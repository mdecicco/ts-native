#include <gjs/lexer/token.h>
#include <stack>
#include <vector>

namespace gjs {
    class script_context;

    namespace parse {
        struct ast;

        struct context {
            script_context* env;
            ast* root;
            std::stack<ast*> path;
            std::vector<lex::token> tokens;
            std::vector<std::string> type_names;
            std::vector<std::pair<std::string, std::vector<std::string>>> named_imports;
            std::stack<u32> cur_stack;
            u32 cur_token;

            context(script_context* ctx);

            bool is_typename(const std::string& text) const;

            bool is_typename() const;

            const lex::token& current() const;
            void consume();
            bool at_end() const;

            void backup();
            void restore();
            void commit();

            // current() == any of the provided strings
            bool match(const std::vector<std::string>& any) const;

            // because apparently sometimes std::string is convertible to lex::token_type...
            bool match_s(const std::vector<std::string>& any) const;

            // current() == any of the provided token types
            bool match(const std::vector<lex::token_type>& any) const;

            // next sizeof(sequence) tokens match the strings in the sequence
            bool pattern(const std::vector<std::string>& sequence) const;

            // because apparently sometimes std::string is convertible to lex::token_type...
            bool pattern_s(const std::vector<std::string>& any) const;

            // next sizeof(sequence) tokens match the token types in the sequence
            bool pattern(const std::vector<lex::token_type>& sequence) const;
        };
    };
};