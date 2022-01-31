#include <gjs/builtin/script_array.h>
#include <gjs/builtin/builtin.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_object.h>
#include <gjs/util/util.h>

#include <gjs/gjs.hpp>

namespace gjs {
    script_array::script_array(u64 moduletype) : m_size(0), m_count(0), m_capacity(0), m_type(nullptr), m_data(nullptr) {
        m_type = resolve_moduletype(moduletype);
    }

    script_array::script_array(u64 moduletype, const script_array& o) {
        m_type = resolve_moduletype(moduletype);
        m_size = o.m_size;
        m_count = o.m_count;
        m_capacity = o.m_capacity;
        m_data = nullptr;

        if (m_capacity > 0) {
            m_data = new u8[u64(m_capacity) * m_type->size];

            if (m_count > 0) {
                for (u32 i = 0;i < m_count;i++) {
                    subtype_t* elem = (subtype_t*)(o.m_data + (u64(i) * u64(m_type->size)));
                    if (m_type->is_trivially_copyable) {
                        if (m_type->is_primitive) memcpy(m_data + (u64(i) * u64(m_type->size)), elem, m_type->size);
                        else memcpy(m_data + (u64(m_count) * u64(m_type->size)), elem, m_type->size);
                    } else {
                        script_object obj(m_type, (u8*)elem);
                        construct_at(m_type, m_data + (u64(m_count) * u64(m_type->size)), obj);
                    }
                }
            }
        }
    }

    script_array::~script_array() {
        if (m_data) {
            if (m_type->destructor) {
                for (u32 i = 0;i < m_count;i++) {
                    call(m_type->destructor, m_data + (u64(i) * u64(m_type->size)));
                }
            }
            delete [] m_data;
            m_data = nullptr;
        }
    }

    void script_array::push(subtype_t* elem) {
        if (m_capacity == 0) {
            m_data = new u8[32 * u64(m_type->size)];
            m_capacity = 32;
        } else if (m_count == m_capacity) {
            m_capacity *= 2;
            u8* newData = new u8[u64(m_capacity) * u64(m_type->size)];
            if (m_type->is_primitive || m_type->is_trivially_copyable) memcpy(newData, m_data, u64(m_count) * u64(m_type->size));
            else {
                for (u32 i = 0;i < m_count;i++) {
                    script_object obj(m_type, m_data + (u64(i) * u64(m_type->size)));
                    construct_at(m_type, newData + (u64(i) * u64(m_type->size)), obj);
                    if (m_type->destructor) call(m_type->destructor, m_data + (u64(i) * u64(m_type->size)));
                }
            }
            delete [] m_data;
            m_data = newData;
        }

        if (m_type->is_trivially_copyable) {
            if (m_type->is_primitive) memcpy(m_data + (u64(m_count) * u64(m_type->size)), elem, m_type->size);
            else memcpy(m_data + (u64(m_count) * u64(m_type->size)), elem, m_type->size);
        } else {
            script_object obj(m_type, (u8*)elem);
            construct_at(m_type, m_data + (u64(m_count) * u64(m_type->size)), obj);
        }
        m_count++;
    }

    void script_array::clear() {
        m_count = 0;
    }

    subtype_t* script_array::operator[](u32 idx) {
        if (idx >= m_count) throw error::runtime_exception(error::ecode::r_array_index_out_of_range, idx, m_count);
        subtype_t* o = (subtype_t*)(m_data + (u64(idx) * u64(m_type->size)));
        return o;
    }

    u32 script_array::length() const {
        return m_count;
    }

    void script_array::for_each(callback<void(*)(u32, subtype_t*)> cb) {
        for (u32 i = 0;i < m_count;i++) {
            // probably a hack not to just use cb(i, (subtype_t*)(m_data + (u64(i) * u64(m_type->size))))
            script_object e = script_object(m_type, (u8*)(m_data + (u64(i) * u64(m_type->size))));
            call(cb.data, i, e);
        }
    }

    bool script_array::some(callback<bool(*)(u32, subtype_t*)> cb) {
        for (u32 i = 0;i < m_count;i++) {
            // probably a hack not to just use cb(i, (subtype_t*)(m_data + (u64(i) * u64(m_type->size))))
            script_object e = script_object(m_type, (u8*)(m_data + (u64(i) * u64(m_type->size))));
            bool result = call(cb.data, i, e);
            if (result) return true;
        }

        return false;
    }

    subtype_t* script_array::find(callback<bool(*)(u32, subtype_t*)> cb, subtype_t* notFoundVal) {
        for (u32 i = 0;i < m_count;i++) {
            // probably a hack not to just use cb(i, (subtype_t*)(m_data + (u64(i) * u64(m_type->size))))
            subtype_t* ptr = (subtype_t*)(m_data + (u64(i) * u64(m_type->size)));
            script_object e = script_object(m_type, (u8*)ptr);
            bool result = call(cb.data, i, e);
            if (result) return ptr;
        }

        return notFoundVal;
    }

    i64 script_array::findIndex(callback<bool(*)(u32, subtype_t*)> cb) {
        for (u32 i = 0;i < m_count;i++) {
            // probably a hack not to just use cb(i, (subtype_t*)(m_data + (u64(i) * u64(m_type->size))))
            script_object e = script_object(m_type, (u8*)(m_data + (u64(i) * u64(m_type->size))));
            bool result = call(cb.data, i, e);
            if (result) return i;
        }
        return -1;
    }
};