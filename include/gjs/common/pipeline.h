#pragma once
#include <vector>
#include <string>
#include <stack>

#include <gjs/common/types.h>
#include <gjs/common/compile_log.h>
#include <gjs/compiler/tac.h>
#include <gjs/backends/stack.h>
#include <gjs/backends/register_allocator.h>

namespace gjs {
    class script_context;
    class script_function;
    class script_enum;
    class backend;
    class script_module;

    namespace parse { struct ast; };

    struct compilation_output {
        struct func_def {
            script_function* func;
            func_stack stack;
            u64 begin;
            u64 end;
            register_allocator regs;
        };
        typedef std::vector<func_def> func_defs;
        typedef std::vector<compile::tac_instruction> ir_code;

        func_defs funcs;
        std::vector<script_type*> types;
        std::vector<script_enum*> enums;
        ir_code code;
        script_module* mod;

        compilation_output(u16 gpN, u16 fpN);

        // will adjust any branch/jump instructions that would be affected
        // by address changes. Also adjusts function boundaries
        void insert(u64 address, const compile::tac_instruction& i);
        void erase(u64 address);
    };


    class pipeline {
        public:
            typedef void (*ir_step_func)(script_context* ctx, compilation_output&);
            typedef void (*ast_step_func)(script_context* ctx, parse::ast*);

            pipeline(script_context* ctx);
            ~pipeline();

            /*
             * 0. Clears logs from previous compilation
             * 1. Parses the source code and generates an abstract syntax tree
             * 2. Executes all of the AST steps on the generated tree (in the order they were added)
             * 3. Generates IR code from the AST
             * 4. Executes all of the IR steps on the generated IR code (in the order they were added)
             * 5. Feeds the resulting IR code after all the IR steps into the provided backend code generator
             *
             * This function may throw the following exceptions:
             *    - parse_exception
             *    - compile_exception
             *    - any exceptions thrown by the ir steps or ast steps
             *
             * Returns the compiled module or null
             */
            script_module* compile(const std::string& module, const std::string& code, backend* generator);

            /*
             * When a module is loaded, the path/name is pushed to the stack
             * to prevent circular imports
             */
            void push_import(const source_ref& ref, const std::string& imported);
            void pop_import();

            /*
             * Takes IR code as a parameter, and modifies it in some way.
             */
            void add_ir_step(ir_step_func step);

            /*
             * Takes the root of the AST as a parameter, and modifies it in some way.
             */
            void add_ast_step(ast_step_func step);

            inline compile_log* log() { return &m_log; }

        protected:
            script_context* m_ctx;
            std::vector<ir_step_func> m_ir_steps;
            std::vector<ast_step_func> m_ast_steps;
            std::vector<std::pair<source_ref, std::string>> m_importStack;
            compile_log m_log;
    };
};

