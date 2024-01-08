#include <tsn/optimize/Optimize.h>
#include <tsn/optimize/OptimizationGroup.h>

#include <tsn/optimize/CommonSubexpressionElimination.h>
#include <tsn/optimize/ConstantFolding.h>
#include <tsn/optimize/DeadCodeElimination.h>
#include <tsn/optimize/CopyPropagation.h>
#include <tsn/optimize/ReduceMemoryAccessStep.h>

namespace tsn {
    namespace optimize {
        IOptimizationStep* defaultOptimizations(Context* ctx) {
            OptimizationGroup* outer = new OptimizationGroup(ctx);

            OptimizationGroup* inner = new OptimizationGroup(ctx);

            inner->addStep(new CopyPropagationStep(ctx));
            inner->addStep(new CommonSubexpressionEliminationStep(ctx));
            inner->addStep(new ReduceMemoryAccessStep(ctx));

            outer->addStep(inner);
            outer->addStep(new ConstantFoldingStep(ctx), true);
            outer->addStep(new DeadCodeEliminationStep(ctx));

            return outer;
        }
    };
};