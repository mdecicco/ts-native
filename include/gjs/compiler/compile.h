#pragma once
#include <gjs/compiler/variable.h>

namespace gjs {
    class script_context;
    class script_type;
    class script_function;
    struct compilation_output;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct context;

        void construct_on_stack(context& ctx, const var& obj, const std::vector<var>& args);

        void construct_in_memory(context& ctx, const var& obj, const std::vector<var>& args);

        void construct_in_place(context& ctx, const var& obj, const std::vector<var>& args);

        // returns module memory offset if global (or static), otherwise UINT64_MAX
        u64 variable_declaration(context& ctx, parse::ast* n);

        script_function* function_declaration(context& ctx, parse::ast* n);

        var function_call(context& ctx, parse::ast* n);

        void add_implicit_destructor_code(context& ctx, parse::ast* n, script_type* cls);

        script_type* class_declaration(context& ctx, parse::ast* n);

        void class_definition(context& ctx, parse::ast* n, script_type* subtype);

        script_type* format_declaration(context& ctx, parse::ast* n);

        void for_loop(context& ctx, parse::ast* n);

        void while_loop(context& ctx, parse::ast* n);

        void do_while_loop(context& ctx, parse::ast* n);

        void import_statement(context& ctx, parse::ast* n);

        void return_statement(context& ctx, parse::ast* n);

        void delete_statement(context& ctx, parse::ast* n);

        void if_statement(context& ctx, parse::ast* n);

        var expression(context& ctx, parse::ast* n);

        void any(context& ctx, parse::ast* n);

        void block(context&ctx, parse::ast* n, bool add_null_if_empty = true);

        void compile(script_context* env, parse::ast* input, compilation_output& out);
    };
};