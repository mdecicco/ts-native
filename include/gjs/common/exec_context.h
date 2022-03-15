#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_tracer.h>

namespace gjs {
    class exec_context {
        public:
            exec_context();
            ~exec_context();

            script_tracer trace;
    };
};

