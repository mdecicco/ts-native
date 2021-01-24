#include <gjs/common/script_object.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_module.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>

namespace gjs {
    script_object::script_object(script_type* type, u8* ptr) {
        m_ctx = type->owner->context();
        m_mod = nullptr;
        m_type = type;
        m_self = ptr;
        m_owns_ptr = false;
        m_destructed = false;
        m_propInfo = nullptr;
        m_refCount = new u32(1);
    }

    script_object::script_object(script_context* ctx, script_module* mod, script_type* type, u8* ptr) {
        m_ctx = ctx;
        m_mod = mod;
        m_type = type;
        m_self = ptr;
        m_owns_ptr = false;
        m_destructed = false;
        m_propInfo = nullptr;
        m_refCount = new u32(1);
    }

    script_object::script_object(const script_object& o) {
        m_ctx = o.m_ctx;
        m_mod = o.m_mod;
        m_type = o.m_type;
        m_self = o.m_self;
        m_owns_ptr = o.m_owns_ptr;
        m_destructed = o.m_destructed;
        m_refCount = o.m_refCount;
        m_propInfo = o.m_propInfo;
        if (m_refCount) (*m_refCount)++;
    }

    script_object::script_object(script_context* ctx) {
        m_ctx = ctx;
        m_mod = nullptr;
        m_type = nullptr;
        m_self = nullptr;
        m_owns_ptr = false;
        m_destructed = false;
        m_propInfo = nullptr;
        m_refCount = new u32(1);
    }

    script_object::script_object(const property_accessor& prop) {
        script_object o = prop.get();
        m_ctx = o.m_ctx;
        m_mod = o.m_mod;
        m_type = o.m_type;
        m_self = o.m_self;
        m_owns_ptr = o.m_owns_ptr;
        m_destructed = o.m_destructed;
        if (!prop.prop->setter) {
            // No reason to keep a record, assign value to
            // m_self directly if operator= is used
            m_propInfo = nullptr;
        }
        else m_propInfo = new property_accessor(prop);
        m_refCount = o.m_refCount;
        if (m_refCount) (*m_refCount)++;
    }

    script_object::~script_object() {
        if (m_refCount && --(*m_refCount) == 0) {
            delete m_refCount;
            m_refCount = nullptr;
            if (m_propInfo) delete m_propInfo;

            if (m_owns_ptr) {
                if (m_destructed) {
                    // runtime error
                    return;
                }
                m_destructed = true;
                if (!m_self) return;

                script_function* dtor = m_type->method("destructor", nullptr, { m_type });
                if (dtor) m_ctx->call(dtor, (void*)m_self);
                if (m_owns_ptr) delete [] m_self;
            }
        }
    }

    script_object script_object::operator [] (const std::string& name) {
        if (!m_self) {
            // todo runtime exception
            return script_object(m_ctx);
        }

        script_type::property* p = m_type->prop(name);
        if (!p) {
            throw error::runtime_exception(error::ecode::r_invalid_object_property, m_type->name.c_str(), name.c_str());
        }

        return script_object({ *this, p });
    }

    bool script_object::is_null() const {
        return m_self == nullptr;
    }

    bool script_object::assign(const script_object& rhs) {
        script_function* cpy_ctor = nullptr;
        if (m_propInfo && m_propInfo->prop->getter) {
            // copy constructor from m_type -> m_type is required if there is a getter
            cpy_ctor = m_type->method("constructor", nullptr, { m_type });
            if (!cpy_ctor) return false;
        }

        if (m_propInfo && m_propInfo->prop->setter) {
            if (rhs.m_type == m_type) {
                m_propInfo->prop->setter->call(m_propInfo->obj, rhs);
            } else {
                // look for applicable copy constructor,
                // create a new object of type m_type from
                // from rhs, pass it to setter

                script_function* ctor = m_type->method("constructor", nullptr, { rhs.m_type });
                if (ctor) m_propInfo->prop->setter->call(m_self, script_object(m_ctx, m_type, rhs));
                else return false;
            }

            if (m_propInfo->prop->getter) {
                // destruct m_self, replace with result of
                // of getter

                if (m_type->destructor) m_type->destructor->call(m_self);
                script_object newVal = m_propInfo->prop->getter->call(m_self);
                cpy_ctor->call(m_self, newVal);
            }

            return true;
        } else {
            // look for operator= on m_type
            script_function* opEq = m_type->method("operator =", nullptr, { rhs.m_type });
            if (opEq) {
                opEq->call(m_self, rhs);
            } else if (rhs.m_type == m_type) {
                // failing that, look for applicable copy
                // constructor, and if it exists, destruct
                // m_self and reconstruct it with rhs

                script_function* cpy_from_ctor = m_type->method("constructor", nullptr, { rhs.m_type });
                if (cpy_from_ctor) {
                    if (m_type->destructor) m_type->destructor->call(m_self);
                    cpy_from_ctor->call(m_self, rhs);
                } else return false;
            }

            if (m_propInfo && m_propInfo->prop->getter) {
                // destruct m_self, replace with result of
                // of getter

                if (m_type->destructor) m_type->destructor->call(m_self);
                script_object newVal = m_propInfo->prop->getter->call(m_self);
                cpy_ctor->call(m_self, newVal);
            }

            return true;
        }

        return false;
    }


    script_object property_accessor::get() const {
        if (!(prop->flags & bind::pf_write_only) && !prop->getter && !prop->setter) {
            u8* ptr = (((u8*)obj.m_self) + prop->offset);
            return script_object(obj.m_ctx, obj.m_mod, prop->type, ptr);
        }

        if (prop->getter) {
            return obj.context()->call(prop->getter, obj.m_self);
        }

        throw error::runtime_exception(error::ecode::r_cannot_read_object_property, prop->name.c_str(), obj.type()->name.c_str());
        return script_object(obj.m_ctx);
    }
};