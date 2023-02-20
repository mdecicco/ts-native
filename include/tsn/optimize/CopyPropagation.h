#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    
    namespace optimize {
        class CodeHolder;

        class CopyPropagationStep : public IOptimizationStep {
            public:
                CopyPropagationStep(Context* ctx);
                virtual ~CopyPropagationStep();

                virtual bool execute(CodeHolder* code, basic_block* block, Pipeline* pipeline);
        };
    };
};