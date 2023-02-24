#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    class Pipeline;
    
    namespace compiler {
        class CodeHolder;
    };
    
    namespace optimize {
        class ReduceMemoryAccessStep : public IOptimizationStep {
            public:
                ReduceMemoryAccessStep(Context* ctx);
                virtual ~ReduceMemoryAccessStep();

                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);
        };
    };
};