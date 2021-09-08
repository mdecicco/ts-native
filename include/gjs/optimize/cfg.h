#pragma once
#include <gjs/optimize/optimize.h>
#include <vector>

namespace gjs {
    struct compilation_output;

    namespace optimize {
        struct cfg;
        struct basic_block {
            address begin;
            address end;
            std::vector<u32> from;
            std::vector<u32> to;

            basic_block* flows_from(u8 idx, cfg* graph);
            basic_block* flows_to(u8 idx, cfg* graph);
            bool is_loop(cfg* graph);
        };

        struct cfg {
            std::vector<basic_block> blocks;

            basic_block* blockAtAddr(address a);
            u32 blockIdxAtAddr(address a);
            void build(compilation_output& in, u16 fidx, label_map& lmap);
        };
    };
};