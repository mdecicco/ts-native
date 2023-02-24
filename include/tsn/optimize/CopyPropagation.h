#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;

    namespace compiler {
        class CodeHolder;
    };
    
    namespace optimize {
        class CopyPropagationStep : public IOptimizationStep {
            public:
                CopyPropagationStep(Context* ctx);
                virtual ~CopyPropagationStep();

                virtual bool execute(compiler::CodeHolder* code, basic_block* block, Pipeline* pipeline);
        };
    };
};