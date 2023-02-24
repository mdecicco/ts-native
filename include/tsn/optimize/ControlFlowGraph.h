#pragma once
#include <tsn/common/types.h>
#include <tsn/optimize/types.h>

#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class CodeHolder;
    };

    namespace optimize {
        class ControlFlowGraph;

        struct basic_block {
            address begin;
            address end;
            utils::Array<address> from;
            utils::Array<address> to;

            basic_block* flows_from(u32 idx, ControlFlowGraph* graph);
            basic_block* flows_to(u32 idx, ControlFlowGraph* graph);
            bool is_loop(ControlFlowGraph* graph);
        };

        class ControlFlowGraph {
            public:
                ControlFlowGraph();
                ControlFlowGraph(compiler::CodeHolder* ch);

                void rebuild(compiler::CodeHolder* ch);
                basic_block* blockAtAddr(address a);
                u32 blockIdxAtAddr(address a);

                utils::Array<basic_block> blocks;
        };
    };
};