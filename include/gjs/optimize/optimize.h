#pragma once
#include <gjs/common/types.h>
#include <gjs/util/robin_hood.h>

namespace gjs {
    class script_context;
    struct compilation_output;

    namespace compile {
        typedef u32 label_id;
    };

    namespace optimize {
        typedef robin_hood::unordered_map<compile::label_id, address> label_map;
        void build_label_map(compilation_output& in, u16 fidx, label_map& lmap);

        void test_step(script_context* ctx, compilation_output& in, u16 fidx);
    };
};