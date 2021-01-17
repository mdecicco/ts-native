#include <gjs/builtin/script_array.h>
#include <gjs/builtin/builtin.h>
#include <gjs/common/script_type.h>
#include <gjs/util/util.h>

namespace gjs {
    script_array::script_array(u64 moduletype) : m_size(0), m_count(0), m_capacity(0), m_type(nullptr), m_data(nullptr) {
        m_type = resolve_moduletype(moduletype);
    }

    script_array::~script_array() {
    }

    void script_array::push(subtype_t* elem) {
        if (m_capacity == 0) {
            m_data = new u8[32 * m_type->size];
            m_capacity = 32;
        }
        memcpy(m_data + (m_count * m_type->size), &elem, m_type->size);
        m_count++;
    }

    subtype_t* script_array::operator[](u32 idx) {
        subtype_t* o = (subtype_t*)(m_data + (idx * m_type->size));
        return o;
    }

    u32 script_array::length() const {
        return m_count;
    }
};