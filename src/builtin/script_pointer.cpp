#include <gjs/builtin/script_pointer.h>
#include <gjs/common/errors.h>
#include <gjs/backends/backend.h>

#include <gjs/gjs.hpp>

namespace gjs {
    script_pointer::script_pointer(u64 moduletype) : m_type(nullptr), m_data(nullptr), m_refCount(nullptr) {
        m_type = resolve_moduletype(moduletype);
    }

    script_pointer::script_pointer(u64 moduletype, const script_pointer& v) : m_type(nullptr), m_data(nullptr), m_refCount(nullptr)  {
        m_type = resolve_moduletype(moduletype);
        share(v);
    }

    script_pointer::~script_pointer() {
        if (m_data) release();
    }

    void script_pointer::reset(subtype_t* v) {
        if (m_type->is_primitive) {
            release();
            m_data = new u8[m_type->size];
            memcpy(m_data, v, m_type->size);
            m_refCount = new u32(1);
            // printf("Created pointer to %s: 0x%lX\n", m_type->name.c_str(), (u64)m_data);
            return;
        }

        // look for applicable copy constructor, and if it
        // exists, destruct m_data and reconstruct it with rhs

        script_function* cpy_from_ctor = m_type->method("constructor", nullptr, { m_type });
        if (cpy_from_ctor) {
            release();
            m_data = new u8[m_type->size];
            m_refCount = new u32(1);
            call<const script_object&>(cpy_from_ctor, m_data, script_object(m_type, (u8*)v));
        }
    }

    void script_pointer::share(const script_pointer& v) {
        if (m_type != v.m_type) {
            throw error::runtime_exception(error::ecode::r_pointer_assign_type_mismatch, m_type->name.c_str(), v.m_type ? v.m_type->name.c_str() : "(null)");
        }

        if (m_data) release();
        m_data = v.m_data;
        m_refCount = v.m_refCount;
        if (m_refCount) {
            (*m_refCount)++;
            // printf("Increased ref count for 0x%lX to %d\n", (u64)m_data, *m_refCount);
        }
    }

    void script_pointer::release() {
        if (m_refCount) {
            (*m_refCount)--;
            // printf("Decreased ref count for 0x%lX to %d\n", (u64)m_data, *m_refCount);
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

    script_pointer& script_pointer::operator=(subtype_t* v) {
        if (!m_data) {
            throw error::runtime_exception(error::ecode::r_cannot_set_null_pointer, m_type->name.c_str());
        }

        if (m_type->is_primitive) {
            memcpy(m_data, v, m_type->size);
            return *this;
        }

        // look for applicable assignment operator, and if it
        // exists, destruct m_data and reconstruct it with rhs
        script_function* assign_op = m_type->method("operator =", nullptr, { m_type });
        if (assign_op) {
            call<const script_object&>(assign_op, m_data, script_object(m_type, (u8*)v));
            return *this;
        }

        // look for applicable copy constructor, and if it
        // exists, destruct m_data and reconstruct it with rhs
        script_function* cpy_from_ctor = m_type->method("constructor", nullptr, { m_type });
        if (cpy_from_ctor) {
            if (m_type->destructor) call(m_type->destructor, (void*)m_data);
            call<const script_object&>(cpy_from_ctor, m_data, script_object(m_type, (u8*)v));
            return *this;
        }


        // todo: runtime error
        return *this;
    }

    script_pointer::operator subtype_t*() {
        if (!m_data) {
            throw error::runtime_exception(error::ecode::r_cannot_get_null_pointer, m_type->name.c_str());
        }
        return (subtype_t*)m_data;
    }

    void script_pointer::destruct() {
        if (m_type->destructor) call(m_type->destructor, (void*)m_data);
        delete [] m_data;
        m_data = nullptr;
    }
};
