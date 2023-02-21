#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    namespace optimize {
        IOptimizationStep::IOptimizationStep(Context* ctx) : IContextual(ctx) {
            m_group = nullptr;
        }

        IOptimizationStep::~IOptimizationStep() {
        }

        void IOptimizationStep::setGroup(OptimizationGroup* group) {
            m_group = group;
        }

        OptimizationGroup* IOptimizationStep::getGroup() const {
            return m_group;
        }

        bool IOptimizationStep::execute(CodeHolder* code, Pipeline* pipeline) {
            return false;
        }
        
        bool IOptimizationStep::execute(CodeHolder* code, basic_block* block, Pipeline* pipeline) {
            return false;
        }
    };
};