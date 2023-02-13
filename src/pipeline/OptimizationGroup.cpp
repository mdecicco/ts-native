#include <tsn/pipeline/OptimizationGroup.h>
#include <utils/Array.hpp>

namespace tsn {
    OptimizationGroup::OptimizationGroup(Context* ctx) : IOptimizationStep(ctx) {
    }

    OptimizationGroup::~OptimizationGroup() {
        m_steps.each([](IOptimizationStep* step) {
            delete step;
        });

        m_steps.clear();
    }

    void OptimizationGroup::setShouldRepeat(bool doRepeat) {
        m_doRepeat = true;
    }

    bool OptimizationGroup::willRepeat() const {
        return m_doRepeat;
    }

    void OptimizationGroup::addStep(IOptimizationStep* step) {
        m_steps.push(step);
    }

    bool OptimizationGroup::execute(CodeHolder* code, Pipeline* pipeline) {
        m_doRepeat = false;

        m_steps.each([code, pipeline](IOptimizationStep* step) {
            while (step->execute(code, pipeline));
        });

        return m_doRepeat;
    }
};