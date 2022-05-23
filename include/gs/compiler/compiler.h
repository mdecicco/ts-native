#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IFunctionHolder.h>
#include <gs/interfaces/IDataTypeHolder.h>

namespace gs {
    namespace compiler {
        struct ast_node;

        class CompilerOutput : public IFunctionHolder, public IDataTypeHolder {
            public:
                CompilerOutput();
                ~CompilerOutput();
        };

        class Compiler {
            public:
                Compiler(ast_node* programTree);
                ~Compiler();

            private:
                ast_node* m_program;
        }
    };
};