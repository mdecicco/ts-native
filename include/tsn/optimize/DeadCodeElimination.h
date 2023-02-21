#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    class CodeHolder;

    namespace optimize {
        class DeadCodeEliminationStep : public IOptimizationStep {
            public:
                DeadCodeEliminationStep(Context* ctx);
                virtual ~DeadCodeEliminationStep();

                virtual bool execute(CodeHolder* code, Pipeline* pipeline);
        };
    };
};