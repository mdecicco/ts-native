#include <gjs/builtin/script_process.h>
#include <gjs/common/errors.h>
#include <gjs/parser/parse_utils.h>

namespace gjs {
    script_process::script_process() {
    }

    script_process::script_process(u32 argc, const char** argv) {
        for (u32 i = 0;i < argc;i++) {
            const char* arg = argv[i];
            m_raw_args.push_back(arg);

            if (*arg == '-') {
                arg++;
                if (*arg == '-') arg++;
                const char* eq = strrchr(arg, '=');

                if (eq) {
                    m_args.push_back({ script_string((void*)arg, (u64)eq - (u64)arg), eq + 1 });
                }
                else {
                    m_args.push_back({ arg, "" });

                    if (argc > i + 1 && *argv[i + 1] != '-') {
                        m_args.back().value = script_string(argv[i + 1]);
                        m_raw_args.push_back(argv[i + 1]);
                        i++;
                    }
                }
            } else {
                m_args.push_back({ "", arg });
            }
        }
    }

    script_process::~script_process() {
    }

    u32 script_process::argc() const {
        return m_args.size();
    }

    const process_arg& script_process::get_arg(u32 idx) const {
        if (idx >= m_args.size()) {
            throw error::runtime_exception(error::ecode::r_array_index_out_of_range, idx, m_args.size());
        }

        return m_args[idx];
    }

    u32 script_process::raw_argc() const {
        return m_raw_args.size();
    }

    const script_string& script_process::get_raw_arg(u32 idx) const {
        if (idx >= m_raw_args.size()) {
            throw error::runtime_exception(error::ecode::r_array_index_out_of_range, idx, m_raw_args.size());
        }

        return m_raw_args[idx];
    }
};
