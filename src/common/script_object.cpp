#include <gjs/common/script_object.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_module.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>

namespace gjs {
    script_object::~script_object() {
        script_function* dtor = m_type->method("destructor", nullptr, { m_type });
        if (dtor) m_ctx->call<void, void*>(dtor, nullptr, m_self);
        delete [] m_self;
    }
};