#include <tsn/optimize/OptimizationGroup.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        OptimizationGroup::OptimizationGroup(Context* ctx) : IOptimizationStep(ctx) {
            m_doRepeat = false;
        }

        OptimizationGroup::~OptimizationGroup() {
            m_steps.each([](IOptimizationStep* step) {
                delete step;
            });

            m_steps.clear();
        }

        void OptimizationGroup::setShouldRepeat(bool doRepeat) {
            m_doRepeat = doRepeat;
        }

        bool OptimizationGroup::willRepeat() const {
            return m_doRepeat;
        }

        void OptimizationGroup::addStep(IOptimizationStep* step, bool isRequired) {
            m_steps.push(step);
            step->setGroup(this);
            step->setRequired(isRequired || step->isRequired());
        }

        bool OptimizationGroup::execute(compiler::CodeHolder* code, Pipeline* pipeline) {
            m_doRepeat = false;

            bool optimizationsDisabled = getContext()->getConfig()->disableOptimizations;
            m_steps.each([code, pipeline, optimizationsDisabled](IOptimizationStep* step) {
                if (optimizationsDisabled && !step->isRequired()) return;

                for (auto& b : code->cfg.blocks) {
                    while (step->execute(code, &b, pipeline));
                }
                
                while (step->execute(code, pipeline));
            });

            return m_doRepeat;
        }
    };
};