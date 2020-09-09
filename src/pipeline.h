#pragma once
#include <vector>
#include <string>
#include <types.h>
#include <compile_log.h>

namespace gjs {
    class vm_context;
    namespace parse {
        struct ast;
    };

    namespace compile {
        struct tac_instruction;
    };

    typedef std::vector<compile::tac_instruction*> ir_code;

    class backend {
        public:
            backend() { }
            virtual ~backend() { }

            /*
             * Takes the final IR code and generates instructions for some target
             * architecture. What it does with that code is out of the scope of
             * the pipeline.
             */
            virtual void generate(const ir_code& ir) = 0;
    };

    class pipeline {
        public:
            typedef void (*ir_step_func)(vm_context* ctx, ir_code&);
            typedef void (*ast_step_func)(vm_context* ctx, parse::ast*);

            pipeline(vm_context* ctx);
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
            vm_context* m_ctx;
            std::vector<ir_step_func> m_ir_steps;
            std::vector<ast_step_func> m_ast_steps;
            compile_log m_log;
    };
};

