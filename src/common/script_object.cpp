#include <common/script_object.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/script_module.h>
#include <backends/backend.h>
#include <common/script_context.h>

namespace gjs {
    script_object::~script_object() {
        script_function* dtor = m_type->method("destructor", nullptr, { m_type });
        if (dtor) m_ctx->call<void, void*>(dtor, nullptr, m_self);
        delete [] m_self;
    }
};