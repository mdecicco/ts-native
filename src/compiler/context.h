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
            struct block_context {
                script_function* func = nullptr;
                std::vector<var> named_vars;
                std::vector<var> stack_objs;
                u32 start = 0;
                u32 end = 0;
            };

            struct deferred_node {
                parse::ast* node = 0;
                script_type* subtype_replacement = 0;
            };

            // todo: comments
            script_context* env;
            parse::ast* input;
            type_manager* new_types;
            std::vector<script_function*> new_functions;
            compilation_output& out;
            bool compiling_function;
            std::vector<tac_instruction> global_code;
            u32 next_reg_id;
            std::vector<parse::ast*> node_stack;
            std::vector<block_context*> block_stack;
            std::vector<deferred_node> deferred;
            std::vector<parse::ast*> subtype_types;
            script_type* subtype_replacement;


            context(compilation_output& out);
            var imm(u64 u);
            var imm(i64 i);
            var imm(f32 f);
            var imm(f64 d);
            var imm(const std::string& s);
            var& empty_var(script_type* type, const std::string& name);
            var empty_var(script_type* type);
            var& dummy_var(script_type* type, const std::string& name);
            var dummy_var(script_type* type);
            var null_var();
            var& error_var();
            var& get_var(const std::string& name);

            // use only to retrieve a function to be called (logs errors when not found or ambiguous)
            script_function* function(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if an identical function exists (does not log errors)
            script_function* find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use to determine if an identifier is in use
            bool identifier_in_use(const std::string& name);

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
            void pop_block(const var& v);
            parse::ast* node();
            block_context* block();
            compile_log* log();
        };
    };
};