#pragma once
#include <gjs/compiler/variable.h>
#include <gjs/compiler/tac.h>
#include <gjs/common/pipeline.h>
#include <gjs/compiler/symbol_table.h>

namespace gjs {
    class script_context;
    class type_manager;
    class compile_log;
    class script_enum;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct context {
            struct block_context {
                script_function* func = nullptr;
                std::list<var> named_vars;
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
            std::vector<script_enum*> new_enums;
            compilation_output& out;
            compilation_output::ir_code global_code;
            u32 next_reg_id;
            std::vector<parse::ast*> node_stack;
            std::vector<block_context*> block_stack;
            std::vector<deferred_node> deferred;
            std::vector<parse::ast*> subtype_types;
            script_type* subtype_replacement;
            bool compiling_function;
            bool compiling_deferred;
            bool compiling_static;
            u64 global_statics_end;
            var error_v;
            std::vector<script_type*> expr_tp_hint;

            symbol_table symbols;


            context(compilation_output& out, script_context* env);
            ~context();

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
            void promote_var(var& v, const std::string& name);
            var force_cast_var(const var& v, script_type* as);

            // use only to retrieve a function to be called (logs errors when not found or ambiguous)
            script_function* function(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only to retrieve a function to be called (logs errors when not found or ambiguous)
            script_function* function(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if an identical function exists (does not log errors)
            script_function* find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if an identical function exists (does not log errors)
            script_function* find_func(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            script_type* type(const std::string& name, bool do_throw = true);
            script_type* type(parse::ast* type_identifier);

            // returns class type if compilation is currently nested within a class
            script_type* class_tp();

            // returns current function if compilation is currently nested within a function
            script_function* current_function();

            tac_wrapper add(operation op);
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