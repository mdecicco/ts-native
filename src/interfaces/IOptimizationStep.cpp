#include <tsn/interfaces/IOptimizationStep.h>

namespace tsn {
    CodeHolder::CodeHolder(utils::Array<compiler::Instruction>& _code) : code(_code) {
    }

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
};