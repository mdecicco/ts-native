#pragma once
#include <tsn/interfaces/IContextual.h>

#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class Instruction;
        class CodeHolder;
    };

    namespace ffi {
        class Function;
    };

    class Pipeline;

    namespace optimize {
        class OptimizationGroup;
        struct basic_block;

        /**
         * @brief Interface for defining optimization steps for IR code. Each optimization
         * step is executed repeatedly until the execution function returns false. This is
         * useful for multipass optimizations. Each optimization can be executed on either
         * the code as a whole, or on each basic block of the control flow graph (in order
         * of execution). If both methods are implemented, the variation that works on the
         * basic blocks is executed first.
         * 
         */
        class IOptimizationStep : public IContextual {
            public:
                IOptimizationStep(Context* ctx);
                virtual ~IOptimizationStep();

                void setGroup(OptimizationGroup* group);
                OptimizationGroup* getGroup() const;

                /**
                 * @brief Executes the optimization pass
                 * 
                 * @param code An object which contains the code to optimize
                 * @param pipeline Pointer to the pipeline which is managing compilation
                 * 
                 * @return Returns true if the pass should be executed again immediately
                 */
                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);

                /**
                 * @brief Executes the optimization pass on a basic block. This is called
                 *        one time per basic block
                 * 
                 * @param code An object which contains the code to optimize
                 * @param block Pointer to a basic block of the control flow graph
                 * @param pipeline Pointer to the pipeline which is managing compilation
                 * 
                 * @return Returns true if the pass should be executed again immediately
                 */
                virtual bool execute(compiler::CodeHolder* code, basic_block* block, Pipeline* pipeline);
            
            private:
                OptimizationGroup* m_group;
        };
    };
};