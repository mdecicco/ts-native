#pragma once
#include <compiler/variable.h>
#include <compiler/tac.h>
#include <common/pipeline.h>

namespace gjs {
    class script_context;
    class type_manager;
    class compile_log;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct context {
            script_context* env;
            parse::ast* input;
            type_manager* new_types;
            std::vector<script_function*> new_functions;
            compilation_output& out;
            u32 next_reg_id;
            std::vector<parse::ast*> node_stack;

            struct block_context {
                script_function* func;
                std::vector<var> named_vars;
                u32 start;
                u32 end;
            };

            // stack is used in case I figure out a way to have arrow functions declared inside of functions
            std::vector<block_context*> func_stack;

            context(compilation_output& out);
            var imm(u64 u);
            var imm(i64 i);
            var imm(f32 f);
            var imm(f64 d);
            var imm(const std::string& s);
            var empty_var(script_type* type, const std::string& name);
            var empty_var(script_type* type);
            var dummy_var(script_type* type, const std::string& name);
            var dummy_var(script_type* type);
            var null_var();
            var error_var();
            var get_var(const std::string& name);

            // use only to retrieve a function to be called (logs errors when not found or ambiguous)
            script_function* function(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if an identical function exists (does not log errors)
            script_function* find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            script_type* type(const std::string& name);
            script_type* type(parse::ast* type_identifier);

            // returns class type if compilation is currently nested within a class
            script_type* class_tp();

            // returns current function if compilation is currently nested within a function
            script_function* current_function();

            tac_instruction& add(operation op);
            void ensure_code_ref();
            u64 code_sz() const;
            void push_node(parse::ast* node);
            void pop_node();
            void push_block(script_function* f = nullptr);
            void pop_block();
            parse::ast* node();
            compile_log* log();
        };
    };
};