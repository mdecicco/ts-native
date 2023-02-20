#pragma once

namespace tsn {
    class Context;

    namespace optimize {
        class IOptimizationStep;
        IOptimizationStep* defaultOptimizations(Context* ctx);
    };
};