#pragma once
#include <tsn/interfaces/IContextual.h>

#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class Instruction;
    };

    class CodeHolder {
        public:
            CodeHolder(utils::Array<compiler::Instruction>& code);

            utils::Array<compiler::Instruction>& code;
    };

    class OptimizationGroup;
    class Pipeline;

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
            virtual bool execute(CodeHolder* code, Pipeline* pipeline) = 0;
        
        private:
            OptimizationGroup* m_group;
    };
};