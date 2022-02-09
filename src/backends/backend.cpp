#include <gjs/backends/backend.h>

namespace gjs {
    backend::backend() : m_ctx(nullptr) {
    }
    backend::~backend() { }

    script_context* backend::context() { return m_ctx; }

    void backend::log_asm(bool do_log) { m_log_asm = do_log; }

    void backend::init() { }
};