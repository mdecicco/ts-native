#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <gjs/builtin/script_string.h>

namespace gjs {
    class script_function;

    struct trace_node {
        function_id func;
        module_id mod;
        u64 ref_offset;
    };

    class script_tracer {
        public:
            script_tracer();
            ~script_tracer();
            
            void expand();
            void produce_error(const script_string& msg);

            u64 node_count;
            u64 node_capacity;
            trace_node* nodes;
            bool has_error;

            script_string error;
    };
};
