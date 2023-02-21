#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    
    namespace optimize {
        class CodeHolder;
        class CommonSubexpressionEliminationStep : public IOptimizationStep {
            public:
                CommonSubexpressionEliminationStep(Context* ctx);
                virtual ~CommonSubexpressionEliminationStep();

                virtual bool execute(CodeHolder* code, basic_block* block, Pipeline* pipeline);
        };
    };
};