#include <gjs/common/compile_log.h>
#include <gjs/common/errors.h>
#include <gjs/common/warnings.h>
#include <stdarg.h>

namespace gjs {
    compile_log::compile_log() {
    }

    compile_log::~compile_log() {
    }

    void compile_log::err(error::ecode code, source_ref at, ...) {
        va_list l;
        va_start(l, at);
        char out[1024] = { 0 };
        vsnprintf(out, 1024, error::format_str(code), l);
        va_end(l);
        
        errors.push_back({ true, code, std::string(out), at });
    }

    void compile_log::warn(warning::wcode code, source_ref at, ...) {
        va_list l;
        va_start(l, at);
        char out[1024] = { 0 };
        vsnprintf(out, 1024, warning::format_str(code), l);
        va_end(l);

        warnings.push_back({ true, (error::ecode)code, std::string(out), at });
    }
};