#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    
    namespace compiler {
        class CodeHolder;
    };
    
    namespace optimize {
        class ConstantFoldingStep : public IOptimizationStep {
            public:
                ConstantFoldingStep(Context* ctx);
                virtual ~ConstantFoldingStep();

                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);
        };
    };
};