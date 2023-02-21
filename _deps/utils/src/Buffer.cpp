#include <utils/Buffer.h>
#include <utils/Allocator.hpp>
#include <utils/String.h>

namespace utils {
    Buffer::Buffer() {
        m_canResize = true;
        m_capacity = 4096;
        m_pos = 0;
        m_used = 0;
        m_data = (u8*)Mem::alloc(m_capacity);
    }

    Buffer::Buffer(u64 capacity) {
        m_canResize = capacity == 0;
        m_capacity = capacity > 0 ? capacity : 4096;
        m_pos = 0;
        m_used = 0;
        m_data = (u8*)Mem::alloc(m_capacity);
    }

    Buffer::~Buffer() {
        if (m_data) Mem::free(m_data);
        m_data = nullptr;
        m_capacity = 0;
        m_pos = 0;
        m_used = 0;
    }

    bool Buffer::write(const void* src, u64 sz) {
        if ((m_pos + sz) > m_capacity) {
            if (!m_canResize) return false;

            m_capacity *= 2;
            u8* data = (u8*)Mem::alloc(m_capacity);
            memcpy(data, m_data, m_used);
            Mem::free(m_data);
            m_data = data;
        }

        memcpy(m_data + m_pos, src, sz);
        m_pos += sz;
        if (m_pos > m_used) m_used = m_pos;

        return true;
    }

    bool Buffer::read(void* dst, u64 sz) {
        if ((m_pos + sz) > m_capacity) return false;

        memcpy(dst, m_data + m_pos, sz);
        m_pos += sz;
        if (m_pos > m_used) m_used = m_pos;

        return true;
    }

    bool Buffer::write(const Buffer* src, u64 sz) {
        if (!src) return false;
        if (sz == UINT64_MAX) sz = src->remaining();
        if (sz == 0) return false;
        if ((src->m_pos + sz) > src->m_capacity) return false;

        return write(src->data(), sz);
    }

    bool Buffer::read(Buffer* dst, u64 sz) {
        return dst->write(this, sz);
    }

    bool Buffer::write(const String& str) {
        static u8 n = 0;
        if (str.size() == 0) return write(&n, 1);
        if (!write((void*)str.c_str(), str.size())) return false;
        return write(&n, 1);
    }

    String Buffer::readStr() {
        if (at_end()) return "";

        String str;
        char ch = 0;
        i32 readSz = 0;
        do {
            if (!read(&ch, 1)) return str;
            if (!ch) break;
            else str += ch;
            readSz++;
        } while (!at_end());

        return str;
    }

    String Buffer::readStr(u32 len) {
        if ((m_pos + len) > m_capacity) return "";

        String str;
        for (u32 i = 0;i < len;i++) {
            char ch = 0;
            if (!read(&ch, 1)) return str;
            str += ch;
        }

        return str;
    }

    u64 Buffer::position(u64 pos) {
        if (pos >= m_capacity) return UINT64_MAX;
        return m_pos = pos;
    }

    void* Buffer::data(u64 offset) {
        if (offset >= m_capacity) return nullptr;
        return m_data + offset;
    }

    const void* Buffer::data(u64 offset) const {
        if (offset >= m_capacity) return nullptr;
        return m_data + offset;
    }


    bool Buffer::canResize() const {
        return m_canResize;
    }

    u64 Buffer::size() const {
        return m_used;
    }

    u64 Buffer::remaining() const {
        return m_used - m_pos;
    }

    u64 Buffer::capacity() const {
        return m_capacity;
    }

    u64 Buffer::position() const {
        return m_pos;
    }

    void* Buffer::data() {
        return m_data + m_pos;
    }

    const void* Buffer::data() const {
        return m_data + m_pos;
    }

    bool Buffer::at_end() const {
        return m_pos == m_used;
    }

    bool Buffer::save(const String& path) const {
        if (m_used == 0 || !m_data) return false;

        FILE* fp = nullptr;
        fopen_s(&fp, path.c_str(), "wb");
        if (!fp) return false;

        if (fwrite(m_data, m_used, 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        fclose(fp);
        return true;
    }

    Buffer* Buffer::FromFile(const String& path) {
        FILE* fp = nullptr;
        fopen_s(&fp, path.c_str(), "rb");
        if (!fp) return nullptr;

        fseek(fp, 0, SEEK_END);
        size_t sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (sz == 0) {
            fclose(fp);
            return nullptr;
        }

        Buffer* buf = new Buffer(sz);
        if (fread(buf->data(), sz, 1, fp) != 1) {
            fclose(fp);
            delete buf;
            return nullptr;
        }

        buf->m_used = buf->m_capacity;

        fclose(fp);
        return buf;
    }
};