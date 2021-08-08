#include <gjs/builtin/script_pointer.h>
#include <gjs/util/util.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_type.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>

namespace gjs {
    script_pointer::script_pointer(u64 moduletype) : m_type(nullptr), m_data(nullptr), m_refCount(new u32(1)) {
        m_type = resolve_moduletype(moduletype);
    }

    script_pointer::~script_pointer() {
        if (m_data) release();
        if (m_refCount) {
            delete m_refCount;
            m_refCount = nullptr;
        }
    }

    void script_pointer::reset(subtype_t* v) {
        if (m_type->is_primitive) {
            release();
            m_data = new u8[m_type->size];
            memcpy(m_data, &v, m_type->size);
            m_refCount = new u32(1);
            return;
        }

        // look for applicable copy constructor, and if it
        // exists, destruct m_data and reconstruct it with rhs

        script_function* cpy_from_ctor = m_type->method("constructor", nullptr, { m_type });
        if (cpy_from_ctor) {
            release();
            m_data = new u8[m_type->size];
            m_refCount = new u32(1);
            cpy_from_ctor->call<const script_object&>(m_data, script_object(m_type, (u8*)v));
        }
    }

    void script_pointer::share(const script_pointer& v) {
        if (m_type != v.m_type) {
            throw error::runtime_exception(error::ecode::r_pointer_assign_type_mismatch, m_type->name.c_str(), v.m_type ? v.m_type->name.c_str() : "(null)");
        }

        if (m_data) release();
        m_data = v.m_data;
        m_refCount = v.m_refCount;
        if (m_refCount) (*m_refCount)++;
    }

    void script_pointer::release() {
        if (m_refCount) {
            (*m_refCount)--;
            if (*m_refCount == 0) {
                if (m_data) destruct();
                delete m_refCount;
            }
            m_data = nullptr;
            m_refCount = nullptr;
        }
    }

    bool script_pointer::is_null() const {
        return m_data == nullptr;
    }

    u32 script_pointer::references() const {
        return m_refCount ? *m_refCount : 0;
    }

    subtype_t* script_pointer::get() const {
        if (!m_data) {
            throw error::runtime_exception(error::ecode::r_cannot_get_null_pointer, m_type->name.c_str());
        }
        return (subtype_t*)m_data;
    }

    script_pointer& script_pointer::operator=(const script_pointer& rhs) {
        share(rhs);
        return *this;
    }

    void script_pointer::destruct() {
        if (m_type->destructor) m_type->destructor->call((void*)m_data);
        delete [] m_data;
        m_data = nullptr;
    }
};