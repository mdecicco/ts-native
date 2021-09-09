#pragma once
#include <gjs/common/types.h>
#include <gjs/util/robin_hood.h>

namespace gjs {
    class script_context;
    struct compilation_output;

    namespace optimize {
        typedef robin_hood::unordered_map<label_id, address> label_map;
        void build_label_map(compilation_output& in, u16 fidx, label_map& lmap);

        // IR optimization phase 1 performs 2 common optimizations:
        // - Copy propagation
        //     - If a register is assigned to the value of another expression with 'a = b'
        //       then all instruction operands that refer to 'a' (apart from operands that
        //       are being assigned) are replaced with 'b'
        // 
        // - Common subexpression elimination
        //     - If multiple registers are assigned to the same binary expression
        //       involving the same registers then it replaces all of them with
        //       simple 'a = b' assignments, where 'b' is the first register assigned
        //       to the expression
        //     - If a register containing the result of some expression is reassigned
        //       before that same expression is discovered again, it is no longer a
        //       candidate for a starting point for this optimization
        //
        // These optimization steps are repeated until no more changes to the IR code are
        // made.
        void ir_phase_1(script_context* ctx, compilation_output& in, u16 fidx);

        // - Dead code elimination
        //     - If a register is assigned a value, and then that register is reassigned
        //       before being read, the assignment instruction is eliminated
        // 
        // This optimization step is repeated until no more changes to the IR code are
        // made.
        void dead_code(script_context* ctx, compilation_output& in, u16 fidx);
    };
};