#include <tsn/compiler/CodeHolder.h>
#include <tsn/compiler/IR.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace compiler {
        CodeHolder::CodeHolder(const utils::Array<compiler::Instruction>& _code) : code(_code) {
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