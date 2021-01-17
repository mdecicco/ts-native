#include <gjs/common/script_object.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_module.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>

namespace gjs {
    property_accessor::operator script_object() {
        // todo: objects

        if (!(prop->flags & bind::pf_write_only) && !prop->getter && !prop->setter) {
            u8* ptr = (((u8*)obj->self()) + prop->offset);
            return script_object(obj->m_ctx, obj->m_mod, prop->type, ptr);
        }

        if (prop->getter) {
            return obj->context()->call(prop->getter, obj->m_self);
        }

        throw error::runtime_exception(error::ecode::r_cannot_read_object_property, prop->name.c_str(), obj->type()->name.c_str());
        return script_object(obj->m_ctx);
    }

    script_object property_accessor::operator =(const script_object& rhs) {
        // todo
        return script_object(obj->m_ctx);
    }

    script_object::script_object(script_context* ctx, script_module* mod, script_type* type, u8* ptr) {
        m_ctx = ctx;
        m_mod = mod;
        m_type = type;
        m_self = ptr;
        m_owns_ptr = false;
        m_destructed = false;
    }

    script_object::script_object(script_context* ctx) {
        m_ctx = ctx;
        m_mod = nullptr;
        m_type = nullptr;
        m_self = nullptr;
        m_owns_ptr = false;
        m_destructed = false;
    }

    script_object::~script_object() {
        if (!m_owns_ptr) destruct();
    }

    property_accessor script_object::prop(const std::string& name) const {
        if (!m_self) {
            // todo runtime exception
        }

        script_type::property* p = m_type->prop(name);
        if (!p) {
            throw error::runtime_exception(error::ecode::r_invalid_object_property, m_type->name.c_str(), name.c_str());
        }

        return { this, p };
    }

    bool script_object::is_null() const {
        return m_self == nullptr;
    }

    void script_object::destruct() {
        if (m_destructed) {
            // runtime error
            return;
        }
        
        script_function* dtor = m_type->method("destructor", nullptr, { m_type });
        if (dtor) m_ctx->call(dtor, (void*)m_self);
        if (m_owns_ptr) delete [] m_self;
        m_destructed = true;
    }
};