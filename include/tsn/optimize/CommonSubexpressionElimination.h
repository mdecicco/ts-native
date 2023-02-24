#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;

    namespace compiler {
        class CodeHolder;
    };
    
    namespace optimize {
        class CommonSubexpressionEliminationStep : public IOptimizationStep {
            public:
                CommonSubexpressionEliminationStep(Context* ctx);
                virtual ~CommonSubexpressionEliminationStep();

                virtual bool execute(compiler::CodeHolder* code, basic_block* block, Pipeline* pipeline);
        };
    };
};