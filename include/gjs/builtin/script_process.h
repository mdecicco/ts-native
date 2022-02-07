#pragma once
#include <gjs/common/types.h>
#include <gjs/builtin/script_string.h>
#include <vector>

namespace gjs {
    struct process_arg {
        script_string name;
        script_string value;
    };

    class script_process {
        public:
            script_process();
            script_process(u32 argc, const char** argv);
            ~script_process();

            u32 argc() const;
            const process_arg& get_arg(u32 idx) const;

            u32 raw_argc() const;
            const script_string& get_raw_arg(u32 idx) const;

            void exit(i32 code) const;

            script_string env(const script_string& name) const;

        protected:
            std::vector<process_arg> m_args;
            std::vector<script_string> m_raw_args;
    };
};

