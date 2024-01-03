#include <tsn/interfaces/IOptimizationStep.h>
#include <tsn/common/types.h>
#include <tsn/optimize/OptimizationGroup.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        IOptimizationStep::IOptimizationStep(Context* ctx) : IContextual(ctx) {
            m_group = nullptr;
            m_isRequired = false;
        }

        IOptimizationStep::~IOptimizationStep() {
        }

        void IOptimizationStep::setGroup(OptimizationGroup* group) {
            m_group = group;
        }
        
        void IOptimizationStep::setRequired(bool required) {
            m_isRequired = required;
            
            if (!m_group) return;
            if (required) {
                m_group->setRequired(true);
                return;
            }

            bool hasRequired = false;
            for (u32 i = 0;i < m_group->m_steps.size() && !hasRequired;i++) {
                hasRequired = m_group->m_steps[i]->m_isRequired;
            }

            if (!hasRequired) m_group->setRequired(false);
        }
        
        bool IOptimizationStep::isRequired() const {
            return m_isRequired;
        }

        OptimizationGroup* IOptimizationStep::getGroup() const {
            return m_group;
        }

        bool IOptimizationStep::execute(compiler::CodeHolder* code, Pipeline* pipeline) {
            return false;
        }
        
        bool IOptimizationStep::execute(compiler::CodeHolder* code, basic_block* block, Pipeline* pipeline) {
            return false;
        }
    };
};