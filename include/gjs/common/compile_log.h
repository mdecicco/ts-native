#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <gjs/util/robin_hood.h>
#include <string>
#include <vector>

namespace gjs {
    namespace error { enum class ecode; };
    namespace warning { enum class wcode; };

    struct compile_message {
        bool is_error;
        union {
            error::ecode e;
            warning::wcode w;
        } code;
        std::string text;
        source_ref src;
    };

    struct log_file {
        std::vector<std::string> lines;
    };

    class compile_log {
        public:
            compile_log();
            ~compile_log();

            void err(error::ecode code, source_ref at, ...);
            void warn(warning::wcode code, source_ref at, ...);

            robin_hood::unordered_map<std::string, log_file> files;
            std::vector<compile_message> errors;
            std::vector<compile_message> warnings;
    };
};

