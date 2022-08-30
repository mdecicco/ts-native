#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/utils/SourceLocation.h>
#include <gs/compiler/Scope.h>

#include <utils/String.h>

namespace gs {
    class Module;

    namespace ffi {
        class DataType;
    };

    namespace compiler {
        struct ast_node;
        class FunctionDef;
        class Compiler;

        class CompilerOutput {
            public:
                CompilerOutput(Compiler* c, Module* m);
                ~CompilerOutput();

                FunctionDef* newFunc(const utils::String& name, ffi::DataType* methodOf = nullptr);
                FunctionDef* getFunc(const utils::String& name, ffi::DataType* methodOf = nullptr);
                const utils::Array<FunctionDef*>& getFuncs() const;

                Module* getModule();

            protected:
                Compiler* m_comp;
                Module* m_mod;
                utils::Array<FunctionDef*> m_funcs;
        };

        class Compiler : public IContextual {
            public:
                Compiler(Context* ctx, ast_node* programTree);
                ~Compiler();

                void enterNode(ast_node* n);
                void exitNode();
                const SourceLocation& getCurrentSrc() const;
                ScopeManager& scope();

                CompilerOutput* getOutput();
                CompilerOutput* compile();

            private:
                ast_node* m_program;
                utils::Array<ast_node*> m_nodeStack;
                CompilerOutput* m_output;
                ScopeManager m_scopeMgr;
        };
    };
};