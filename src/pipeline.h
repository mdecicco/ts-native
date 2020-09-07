#pragma once
#include <vector>
#include <string>
#include <types.h>
#include <compile_log.h>

namespace gjs {
    class vm_context;
    class instruction_array;
    struct source_map;
    namespace parse {
        struct ast;
    };

    class pipeline {
        public:
            typedef void (*ir_step_func)(vm_context* ctx, instruction_array&, source_map*, u32);
            typedef void (*ast_step_func)(vm_context* ctx, parse::ast*);

            pipeline(vm_context* ctx);
            ~pipeline();

            /*
             * 0. Clears logs from previous compilation
             * 1. Parses the source code and generates an abstract syntax tree
             * 2. Executes all of the AST steps on the generated tree (in the order they were added)
             * 3. Generates the source map and intermediate representation (VM code/instruction_array) code from the AST
             * 4. Executes all of the IR steps on the generated IR code (in the order they were added)
             *
             * This function may throw the following exceptions:
             *    - parse_exception
             *    - compile_exception
             *    - any exceptions thrown by the ir steps or ast steps
             */
            bool compile(const std::string& file, const std::string& code);

            /*
             * Takes IR code holder and source map as parameters, and modifies them in some way. Typically
             * would do some kind of optimization.
             *
             * Any added/removed instructions must also result in the addition or removal of the related
             * source map data
             */
            void add_ir_step(ir_step_func step);

            /*
             * Takes the root of the AST as a parameter, and modifies it in some way. Typically would do
             * some kind of optimization or validation
             */
            void add_ast_step(ast_step_func step);

            inline compile_log* log() { return &m_log; }

        protected:
            vm_context* m_ctx;
            std::vector<ir_step_func> m_ir_steps;
            std::vector<ast_step_func> m_ast_steps;
            compile_log m_log;
    };
};

