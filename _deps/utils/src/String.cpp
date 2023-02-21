#include <utils/String.h>
#include <utils/Singleton.hpp>
#include <utils/Allocator.hpp>
#include <utils/Array.hpp>
#include <utils/robin_hood.h>

#include <stdarg.h>

namespace std {
    size_t hash<utils::String>::operator()(const utils::String& str) const {
        return robin_hood::hash_bytes(str.m_data, str.m_len);
    }
};

namespace utils {
    String::String() : m_data(nullptr), m_len(0), m_capacity(0), m_readOnly(false) {
    }

    String::String(const char* str) : m_data(nullptr), m_len(u32(strlen(str))), m_capacity(m_len), m_readOnly(false) {
        m_data = (char*)Allocator::Get()->alloc(m_capacity + u64(1));
        memcpy(m_data, str, m_len);
        m_data[m_len] = 0;
    }

    String::String(const std::string& str) : m_data(nullptr), m_len(u32(str.size())), m_capacity(m_len), m_readOnly(false) {
        m_data = (char*)Allocator::Get()->alloc(m_capacity + u64(1));
        memcpy(m_data, str.c_str(), str.length());
        m_data[m_len] = 0;
    }

    String::String(const String& str) : m_data(nullptr), m_len(str.size()), m_capacity(m_len), m_readOnly(false) {
        if (str.m_readOnly) {
            m_data = str.m_data;
            m_capacity = str.m_capacity;
            m_len = str.m_len;
            m_readOnly = str.m_readOnly;
        } else {
            m_data = (char*)Allocator::Get()->alloc(m_capacity + u64(1));
            memcpy(m_data, str.c_str(), str.size());
            m_data[m_len] = 0;
        }
    }

    String::String(char* str, u32 len) : m_data(nullptr), m_len(len), m_capacity(len), m_readOnly(false) {
        m_data = (char*)Allocator::Get()->alloc(m_capacity + u32(1));
        memcpy(m_data, str, len);
        m_data[m_len] = 0;
    }

    String::~String() {
        if (m_data && !m_readOnly) Allocator::Get()->free(m_data);
        m_data = nullptr;
    }

    u32 String::size() const {
        return m_len;
    }

    char& String::operator[](u32 idx) {
        return m_data[idx];
    }

    char String::operator[](u32 idx) const {
        return m_data[idx];
    }
    
    String& String::operator =(const char* rhs) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }

        u32 slen = (u32)strlen(rhs);
        u32 cap = slen >= 32 ? slen + 32 : 32;
        if (slen > 0 && slen < m_capacity) {
            memcpy(m_data, rhs, slen);
            m_len = slen;
            m_data[m_len] = 0;
            resize(cap);
        } else {
            resize(cap);
            memcpy(m_data, rhs, slen);
            m_len = slen;
            m_data[m_len] = 0;
        }

        return *this;
    }

    String& String::operator =(const std::string& rhs) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }

        u32 cap = u32(rhs.length() >= 32 ? rhs.length() + 32 : 32);
        if (rhs.length() > 0 && rhs.length() < m_capacity) {
            memcpy(m_data, rhs.c_str(), rhs.length());
            m_len = u32(rhs.length());
            m_data[m_len] = 0;
            resize(cap);
        } else {
            resize(cap);
            memcpy(m_data, rhs.c_str(), rhs.length());
            m_len = u32(rhs.length());
            m_data[m_len] = 0;
        }

        return *this;
    }

    String& String::operator =(const String& rhs) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }

        u32 cap = u32(rhs.size() >= 32 ? rhs.size() + 32 : 32);
        if (rhs.size() > 0 && rhs.size() < m_capacity) {
            memcpy(m_data, rhs.m_data, rhs.m_len);
            m_len = rhs.m_len;
            m_data[m_len] = 0;
            resize(cap);
        } else {
            resize(cap);
            memcpy(m_data, rhs.m_data, rhs.m_len);
            m_len = rhs.m_len;
            m_data[m_len] = 0;
        }

        return *this;
    }

    String& String::operator +=(const String& rhs) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }
        
        if ((m_len + rhs.size() + 1) < m_capacity) {
            memcpy(m_data + m_len, rhs.m_data, rhs.m_len);
            m_len += rhs.m_len;
            m_data[m_len] = 0;
        } else {
            resize(m_len + rhs.m_len + 32);
            memcpy(m_data + m_len, rhs.m_data, rhs.m_len);
            m_len += rhs.m_len;
            m_data[m_len] = 0;
        }
        return *this;
    }

    String String::operator +(const String& rhs) const {
        String s(*this);
        s += rhs;
        return s;
    }

    String& String::operator +=(char ch) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }

        if ((m_len + 1) >= m_capacity) resize(m_capacity + 32);

        m_data[m_len++] = ch;
        return *this;
    }

    String String::operator +(char ch) const {
        String s(*this);
        s += ch;
        return s;
    }

    bool String::operator==(const char* rhs) const {
        u32 slen = (u32)strlen(rhs);
        if (slen != m_len) return false;
        for (u32 i = 0;i < slen;i++) {
            if (m_data[i] != rhs[i]) return false;
        }

        return true;
    }

    bool String::operator==(const String& rhs) const {
        if (rhs.m_len != m_len) return false;
        for (u32 i = 0;i < rhs.m_len;i++) {
            if (m_data[i] != rhs.m_data[i]) return false;
        }

        return true;
    }

    void String::replaceAll(const String& str, const String& with) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }
        
        u32 slen = str.size();
        if (slen == 0 || slen > m_len) return;

        Array<u32> indices = indicesOf(str);
        if (indices.size() == 0) return;

        u32 newLength = (m_len - (indices.size() * slen)) + (indices.size() * with.m_len);
        String tmp;
        tmp.resize(newLength + 1);

        u32 iidx = 0;
        for (u32 i = 0;i < m_len;i++) {
            if (iidx < indices.size() && i == indices[iidx]) {
                for (u32 j = 0;j < with.m_len;j++) tmp.m_data[tmp.m_len++] = with.m_data[j];
                i += (slen - 1);
                iidx++;
                continue;
            }

            tmp.m_data[tmp.m_len++] = m_data[i];
        }

        Allocator::Get()->free(m_data);
        m_data = tmp.m_data;
        m_len = tmp.m_len;
        m_capacity = tmp.m_capacity;
        m_data[m_len] = 0;

        tmp.m_data = nullptr;
        tmp.m_len = tmp.m_capacity = 0;
    }

    void String::replaceAll(const char* str, const String& with) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }
        
        u32 slen = (u32)strlen(str);
        if (slen == 0 || slen > m_len) return;

        Array<u32> indices = indicesOf(str);
        if (indices.size() == 0) return;

        u32 newLength = (m_len - (indices.size() * slen)) + (indices.size() * with.m_len);
        String tmp;
        tmp.resize(newLength + 1);

        u32 iidx = 0;
        for (u32 i = 0;i < m_len;i++) {
            if (iidx < indices.size() && i == indices[iidx]) {
                for (u32 j = 0;j < with.m_len;j++) tmp.m_data[tmp.m_len++] = with.m_data[j];
                i += (slen - 1);
                iidx++;
                continue;
            }

            tmp.m_data[tmp.m_len++] = m_data[i];
        }

        Allocator::Get()->free(m_data);
        m_data = tmp.m_data;
        m_len = tmp.m_len;
        m_capacity = tmp.m_capacity;
        m_data[m_len] = 0;

        tmp.m_data = nullptr;
        tmp.m_len = tmp.m_capacity = 0;
    }

    void String::replaceAll(char ch, char with) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }
        
        for (u32 i = 0;i < m_len;i++) {
            if (m_data[i] == ch) m_data[i] = with;
        }
    }

    String String::trim() const {
        String out;
        out.resize(m_len);

        char buf[1024];
        u32 bidx = 0;

        bool firstNonW = false;
        for (u32 i = 0;i < m_len;i++) {
            if (!firstNonW && isspace(m_data[i])) continue;
            firstNonW = true;

            if (isspace(m_data[i])) {
                buf[bidx++] = m_data[i];
                buf[bidx] = 0;
            }
            else {
                if (bidx) {
                    for (u32 b = 0;b < bidx;b++) out.m_data[out.m_len++] = buf[b];
                    bidx = 0;
                }
                out.m_data[out.m_len++] = m_data[i];
            }
        }

        return out;
    }

    String String::clone() const {
        return String(m_data, m_len);
    }

    String String::toUpperCase() const {
        String out(*this);
        for (u32 i = 0;i < m_len;i++) out.m_data[i] = toupper(out.m_data[i]);
        return out;
    }

    String String::toLowerCase() const {
        String out(*this);
        for (u32 i = 0;i < m_len;i++) out.m_data[i] = tolower(out.m_data[i]);
        return out;
    }

    Array<String> String::split(std::initializer_list<const char*> delimiters, const char* notBetween, u32 maxCount) const {
        Array<String> out;
        if (m_len == 0) return out;
        u32 nbLen = notBetween ? (u32)strlen(notBetween) : 0;
        if (nbLen % 2 != 0) return out;

        u32 bidx = 0;
        char buf[1024];
        buf[0] = 0;

        bool hasMax = maxCount > 0;
        char resumeSplitAfter = 0;

        for (u32 i = 0;i < m_len;i++) {
            if (resumeSplitAfter) {
                buf[bidx++] = m_data[i];
                buf[bidx] = 0;

                if (m_data[i] == resumeSplitAfter) resumeSplitAfter = 0;
                continue;
            }

            if (notBetween) {
                for (u32 j = 0;j < nbLen && !resumeSplitAfter;j += 2) {
                    if (m_data[i] == notBetween[j]) {
                        resumeSplitAfter = notBetween[j + 1];
                    }
                }

                if (resumeSplitAfter) {
                    buf[bidx++] = m_data[i];
                    buf[bidx] = 0;
                    continue;
                }
            }

            bool matchedAny = false;
            u32 maxMatchEndPos = 0;

            if (!hasMax || maxCount > 0) {
                for (const char* d : delimiters) {
                    u32 slen = (u32)strlen(d);
                    if (slen == 0 || slen > m_len) return out;

                    bool match = true;
                    u32 j = 0;
                    for (;j < slen && match;j++) match = m_data[i + j] == d[j];
                    if (match) {
                        matchedAny = true;
                        if ((i + slen) > maxMatchEndPos) maxMatchEndPos = i + slen;
                    }
                }
            }

            if (!matchedAny) {
                buf[bidx++] = m_data[i];
                buf[bidx] = 0;
            }
            else {
                out.push(buf);
                bidx = 0;
                i = maxMatchEndPos - 1;
                maxCount--;
            }
        }

        if (bidx > 0) out.push(buf);

        return out;
    }

    Array<String> String::split(const char* delimiter, const char* notBetween, u32 maxCount) const {
        Array<String> out;
        u32 slen = (u32)strlen(delimiter);
        if (slen == 0 || slen > m_len) return out;
        u32 nbLen = notBetween ? (u32)strlen(notBetween) : 0;
        if (nbLen % 2 != 0) return out;

        u32 bidx = 0;
        char buf[1024];
        buf[0] = 0;

        char resumeSplitAfter = 0;

        bool hasMax = maxCount > 0;

        for (u32 i = 0;i < m_len;i++) {
            if (resumeSplitAfter) {
                buf[bidx++] = m_data[i];
                buf[bidx] = 0;

                if (m_data[i] == resumeSplitAfter) resumeSplitAfter = 0;
                continue;
            }

            if (notBetween) {
                for (u32 j = 0;j < nbLen && !resumeSplitAfter;j += 2) {
                    if (m_data[i] == notBetween[j]) {
                        resumeSplitAfter = notBetween[j + 1];
                    }
                }

                if (resumeSplitAfter) {
                    buf[bidx++] = m_data[i];
                    buf[bidx] = 0;
                    continue;
                }
            }

            bool match = true;
            if (!hasMax || maxCount > 0) {
                u32 j = 0;
                for (;j < slen && match;j++) match = m_data[i + j] == delimiter[j];
            } else if (hasMax) match = false;

            if (!match) {
                buf[bidx++] = m_data[i];
                buf[bidx] = 0;
            } else {
                out.push(buf);
                bidx = 0;
                i += slen - 1;
                maxCount--;
            }
        }

        if (bidx > 0) out.push(buf);

        return out;
    }

    Array<u32>String:: indicesOf(const String& str) const {
        Array<u32> out;
        if (str.m_len == 0 || str.m_len > m_len) return out;

        for (u32 i = 0;i < m_len;i++) {
            bool match = true;
            for (u32 j = 0;j < str.m_len && match;j++) match = m_data[i + j] == str.m_data[j];
            if (match) out.push(i);
        }

        return out;
    }

    Array<u32> String::indicesOf(const char* str) const {
        u32 slen = (u32)strlen(str);
        Array<u32> out;
        if (slen == 0 || slen > m_len) return out;

        for (u32 i = 0;i < m_len;i++) {
            bool match = true;
            for (u32 j = 0;j < slen && match;j++) match = m_data[i + j] == str[j];
            if (match) out.push(i);
        }

        return out;
    }

    Array<u32> String::indicesOf(char c) const {
        Array<u32> out;
        if (m_len == 0) return out;
        
        for (u32 i = 0;i < m_len;i++) {
            if (m_data[i] == c) out.push(i);
        }

        return out;
    }

    i64 String::firstIndexOf(const String& str) const {
        if (str.m_len == 0 || str.m_len > m_len) return -1;

        size_t matchC = 0;

        for (u32 i = 0;i < m_len;i++) {
            char a = m_data[i];
            char b = str.m_data[matchC];

            if (a == b) matchC++;
            else matchC = 0;

            if (matchC == str.m_len) return i;
        }

        return -1;
    }

    i64 String::firstIndexOf(const char* str) const {
        u32 slen = (u32)strlen(str);
        if (slen == 0 || slen > m_len) return -1;

        size_t matchC = 0;

        for (u32 i = 0;i < m_len;i++) {
            char a = m_data[i];
            char b = str[matchC];

            if (a == b) matchC++;
            else matchC = 0;

            if (matchC == slen) return i;
        }

        return -1;
    }

    i64 String::firstIndexOf(char c) const {
        for (u32 i = 0;i < m_len;i++) {
            if (m_data[i] == c) return i;
        }

        return -1;
    }

    i64 String::lastIndexOf(const String& str) const {
        if (str.m_len == 0 || str.m_len > m_len) return -1;
        size_t matchC = 0;

        for (i64 i = m_len - 1;i >= 0;i--) {
            char a = m_data[i];
            char b = str.m_data[(str.m_len - 1) - matchC];

            if (a == b) matchC++;
            else matchC = 0;

            if (matchC == str.m_len) return i;
        }

        return -1;
    }

    i64 String::lastIndexOf(const char* str) const {
        u32 slen = (u32)strlen(str);
        if (slen == 0 || slen > m_len) return -1;

        size_t matchC = 0;

        for (i64 i = m_len - 1;i >= 0;i--) {
            char a = m_data[i];
            char b = str[(slen - 1) - matchC];

            if (a == b) matchC++;
            else matchC = 0;
            
            if (matchC == slen) return i;
        }

        return -1;
    }

    i64 String::lastIndexOf(char c) const {
        for (i64 i = m_len - 1;i >= 0;i--) {
            if (m_data[i] == c) return i;
        }

        return -1;
    }

    String String::Format(const char* fmt, ...) {
        char out[1024] = { 0 };
        va_list l;
        va_start(l, fmt);
        i32 len = vsnprintf(out, 1024, fmt, l);
        va_end(l);

        return out;
    }
    
    const String String::View(const char* str, u32 length) {
        String out;
        if (!str) {
            out.m_data = nullptr;
            out.m_capacity = out.m_len = 0;
            out.m_readOnly = true;
        } else {
            out.m_data = const_cast<char*>(str);
            out.m_capacity = 0;
            out.m_len = length ? length : u32(strlen(str));
            out.m_readOnly = true;
        }
        return out;
    }

    void String::resize(u32 cap) {
        if (m_readOnly) {
            throw std::exception("Attempted to modify read-only string");
        }

        m_capacity = cap;
        char* data = (char*)Allocator::Get()->alloc(cap);
        if (m_len < m_capacity) memset(data + m_len, 0, m_capacity - m_len);
        if (m_len) memcpy(data, m_data, m_len);
        if (m_data) Allocator::Get()->free(m_data);
        m_data = data;
    }

    
    String operator+(const char* lhs, const String& rhs) {
        return String(lhs) + rhs;
    }
};