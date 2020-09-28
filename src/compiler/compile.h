#pragma once
#include <compiler/variable.h>

namespace gjs {
    class script_context;
    struct compilation_output;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct context;

        void construct_on_stack(context& ctx, const var& obj, const std::vector<var>& args);

        void construct_in_memory(context& ctx, const var& obj, const std::vector<var>& args);

        void variable_declaration(context& ctx, parse::ast* n);

        void function_declaration(context& ctx, parse::ast* n);

        var function_call(context& ctx, parse::ast* n);

        void class_declaration(context& ctx, parse::ast* n);

        void format_declaration(context& ctx, parse::ast* n);

        void for_loop(context& ctx, parse::ast* n);

        void while_loop(context& ctx, parse::ast* n);

        void do_while_loop(context& ctx, parse::ast* n);

        void import_statement(context& ctx, parse::ast* n);

        void export_statement(context& ctx, parse::ast* n);

        void return_statement(context& ctx, parse::ast* n);

        void delete_statement(context& ctx, parse::ast* n);

        void if_statement(context& ctx, parse::ast* n);

        var expression(context& ctx, parse::ast* n);

        void any(context& ctx, parse::ast* n);

        void block(context&ctx, parse::ast* n);

        void compile(script_context* env, parse::ast* input, compilation_output& out);
    };
};