#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    class CodeHolder;
    
    namespace optimize {
        class ConstantFoldingStep : public IOptimizationStep {
            public:
                ConstantFoldingStep(Context* ctx);
                virtual ~ConstantFoldingStep();

                virtual bool execute(CodeHolder* code, Pipeline* pipeline);
        };
    };
};