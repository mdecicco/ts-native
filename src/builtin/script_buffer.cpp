#include <gjs/builtin/script_buffer.h>
#include <gjs/builtin/builtin.h>
#include <gjs/builtin/script_string.h>
#include <gjs/common/errors.h>

using namespace gjs::error;

namespace gjs {
    script_buffer::script_buffer() : m_can_resize(true), m_capacity(4096), m_data(nullptr), m_position(0), m_used(0) {
        m_data = (u8*)script_allocate(m_capacity);
    }

    script_buffer::script_buffer(u64 capacity) : m_can_resize(capacity == 0), m_capacity(capacity), m_data(nullptr), m_position(0), m_used(0) {
        if (m_capacity == 0) m_capacity = 4096;
        m_data = (u8*)script_allocate(m_capacity);
    }

    script_buffer::~script_buffer() {
        if (m_data) script_free(m_data);
        m_data = nullptr;
    }

    void script_buffer::write(void* data, u64 sz) {
        if ((m_position + sz) > m_capacity) {
            if (m_can_resize) {
                m_capacity *= 2;
                u8* data = (u8*)script_allocate(m_capacity);
                memcpy(data, m_data, m_used);
                script_free(m_data);
                m_data = data;
            } else throw runtime_exception(ecode::r_buffer_overflow, sz, m_used - m_position);
        }

        memcpy(m_data + m_position, data, sz);
        m_position += sz;
        if (m_position > m_used) m_used = m_position;
    }

    void script_buffer::read(void* out, u64 sz) {
        if ((m_position + sz) > m_capacity) throw runtime_exception(ecode::r_buffer_read_out_of_range, sz, m_used - m_position);
        memcpy(out, m_data + m_position, sz);
        m_position += sz;
        if (m_position > m_used) m_used = m_position;
    }

    void script_buffer::write(script_buffer* buf, u64 sz) {
        if ((buf->m_position + sz) > buf->m_capacity) throw runtime_exception(ecode::r_buffer_read_out_of_range, sz, buf->remaining());
        write(buf->data(), sz);
    }

    void script_buffer::read(script_buffer* buf, u64 sz) {
        buf->write(this, sz);
    }

    void script_buffer::write(script_string* str) {
        write((void*)str->c_str(), str->length());
    }

    void script_buffer::read(script_string* str) {
        if (at_end()) return;
        char ch = 0;
        do {
            read<char>(ch);
            if (!ch) break;
            else str->operator+=(ch);
        } while (!at_end());
    }

    void script_buffer::read(script_string* str, u32 len) {
        if ((m_position + len) > m_capacity) throw runtime_exception(ecode::r_buffer_read_out_of_range, len, m_used - m_position);

        for (u32 i = 0;i < len;i++) {
            char ch = 0;
            read<char>(ch);
            str->operator+=(ch);
        }
    }

    u64 script_buffer::position(u64 pos) {
        if (pos >= m_capacity) throw runtime_exception(ecode::r_buffer_offset_out_of_range, pos, m_capacity);
        m_position = pos;
        return m_position;
    }

    void* script_buffer::data(u64 offset) {
        if (offset >= m_capacity) throw runtime_exception(ecode::r_buffer_offset_out_of_range, offset, m_capacity);
        return m_data + offset;
    }
};