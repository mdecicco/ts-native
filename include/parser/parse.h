#include <common/types.h>
#include <lexer/token.h>
#include <vector>
#include <stack>

namespace gjs {
    class script_context;
    enum class error_code;

    namespace parse {
        struct ast;
        struct context;

        ast* identifier(context& ctx);

        ast* type_identifier(context& ctx);

        ast* expression(context& ctx);

        ast* variable_declaration(context& ctx);

        ast* class_declaration(context& ctx);

        ast* function_declaration(context& ctx);

        ast* format_declaration(context& ctx);

        ast* argument_list(context& ctx);

        ast* if_statement(context& ctx);

        ast* return_statement(context& ctx);

        ast* delete_statement(context& ctx);

        ast* import_statement(context& ctx);

        ast* while_loop(context& ctx);

        ast* for_loop(context& ctx);

        ast* do_while_loop(context& ctx);

        ast* block(context& ctx);

        ast* any(context& ctx);

        ast* parse(script_context* env, const std::string& module, const std::vector<lex::token>& tokens);
    };
};