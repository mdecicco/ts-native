#pragma once
#include <tsn/interfaces/IOptimizationStep.h>

#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class CodeHolder;
    };
    
    namespace optimize {
        /**
         * @brief Contains a series of optimization steps. Steps are executed in the order they are added in.
         *        If the group should be executed multiple times, one or more of the added steps should call
         *        `getGroup()->setShouldRepeat(true)`.
         * 
         */
        class OptimizationGroup : public IOptimizationStep {
            public:
                OptimizationGroup(Context* ctx);
                virtual ~OptimizationGroup();

                void setShouldRepeat(bool doRepeat);
                bool willRepeat() const;

                /**
                 * @brief Adds a new optimization step to be executed
                 * 
                 * @param step The step to execute
                 */
                void addStep(IOptimizationStep* step, bool isRequired = false);

                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);
            
            protected:
                friend class IOptimizationStep;
                bool m_doRepeat;
                utils::Array<IOptimizationStep*> m_steps;
        };
    };
};