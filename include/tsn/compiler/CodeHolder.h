#pragma once
#include <tsn/interfaces/IContextual.h>
#include <tsn/optimize/LabelMap.h>
#include <tsn/optimize/ControlFlowGraph.h>
#include <tsn/optimize/LivenessData.h>

#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class Instruction;
    };

    namespace ffi {
        class Function;
    };

    namespace compiler {
        class CodeHolder {
            public:
                CodeHolder(const utils::Array<compiler::Instruction>& code);

                void rebuildAll();
                void rebuildLabels();
                void rebuildCFG();
                void rebuildLiveness();

                ffi::Function* owner;

                optimize::LabelMap labels;
                optimize::ControlFlowGraph cfg;
                optimize::LivenessData liveness;
                utils::Array<compiler::Instruction> code;
        };
    };
};
