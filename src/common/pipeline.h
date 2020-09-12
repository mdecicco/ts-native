#pragma once
#include <vector>
#include <string>
#include <common/types.h>
#include <common/compile_log.h>
#include <compiler/tac.h>

namespace gjs {
    class script_context;
    class script_function;
    class backend;

    namespace parse { struct ast; };

    struct compilation_output {
        struct func_def {
            script_function* func;
            u64 begin;
            u64 end;
        };
        typedef std::vector<func_def> func_defs;
        typedef std::vector<compile::tac_instruction> ir_code;

        func_defs funcs;
        std::vector<script_type*> types;
        ir_code code;

        // will adjust any branch/jump instructions that would be affected
        // by address changes
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
             */
            bool compile(const std::string& file, const std::string& code, backend* generator);

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
            compile_log m_log;
    };
};

