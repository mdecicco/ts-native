#include <tsn/optimize/CodeHolder.h>

namespace tsn {
    namespace optimize {
        CodeHolder::CodeHolder(utils::Array<compiler::Instruction>& _code) : code(_code) {
        }

        void CodeHolder::rebuildAll() {
            labels.rebuild(this);
            cfg.rebuild(this);
            liveness.rebuild(this);
        }

        void CodeHolder::rebuildLabels() {
            labels.rebuild(this);
        }

        void CodeHolder::rebuildCFG() {
            cfg.rebuild(this);
        }

        void CodeHolder::rebuildLiveness() {
            liveness.rebuild(this);
        }
    };
};