#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;

    namespace compiler {
        class CodeHolder;
    };

    namespace optimize {
        class DeadCodeEliminationStep : public IOptimizationStep {
            public:
                DeadCodeEliminationStep(Context* ctx);
                virtual ~DeadCodeEliminationStep();

                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);
        };
    };
};