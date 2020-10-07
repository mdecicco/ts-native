#include <builtin/script_string.h>
#include <builtin/builtin.h>

namespace gjs {
    script_string::script_string() : m_data(nullptr), m_len(0), m_capacity(32) {
        m_data = (char*)script_allocate(m_capacity);
        memset(m_data, 0, m_capacity);
    }

    script_string::script_string(const std::string& str) : m_data(nullptr), m_len(str.length()), m_capacity(m_len) {
        m_data = (char*)script_allocate(m_capacity + u64(1));
        memcpy(m_data, str.c_str(), str.length());
        m_data[m_len] = 0;
    }

    script_string::script_string(const script_string& str) : m_data(nullptr), m_len(str.length()), m_capacity(m_len) {
        m_data = (char*)script_allocate(m_capacity + u64(1));
        memcpy(m_data, str.c_str(), str.length());
        m_data[m_len] = 0;
    }

    script_string::script_string(char* str, u64 len) : m_data(nullptr), m_len(len), m_capacity(len) {
        m_data = (char*)script_allocate(m_capacity + u64(1));
        memcpy(m_data, str, len);
        m_data[m_len] = 0;
    }

    script_string::~script_string() {
        if (m_data) script_free(m_data);
        m_data = nullptr;
    }

    u64 script_string::length() const {
        return m_len;
    }

    char& script_string::operator[](u64 idx) {
        return m_data[idx];
    }

    script_string& script_string::operator =(const std::string& rhs) {
        u64 cap = rhs.length() > 32 ? rhs.length() + 32 : 32;
        if (rhs.length() > 0 && rhs.length() < m_capacity) {
            memcpy(m_data, rhs.c_str(), rhs.length());
            m_len = rhs.length();
            resize(cap);
        } else {
            resize(cap);
            memcpy(m_data, rhs.c_str(), rhs.length());
            m_len = rhs.length();
        }
        return *this;
    }

    script_string& script_string::operator =(const script_string& rhs) {
        if (rhs.m_len > 0 && rhs.m_len < m_capacity) {
            memcpy(m_data, rhs.m_data, rhs.m_len);
            m_len = rhs.m_len;
            resize(rhs.m_capacity);
        } else {
            resize(rhs.m_capacity);
            memcpy(m_data, rhs.m_data, rhs.m_len);
            m_len = rhs.m_len;
        }
        return *this;
    }

    script_string& script_string::operator +=(const script_string& rhs) {
        if ((m_len + rhs.length()) < m_capacity) {
            memcpy(m_data + m_len, rhs.m_data, rhs.m_len);
            m_len += rhs.m_len;
        } else {
            resize(m_len + rhs.m_len + 32);
            memcpy(m_data + m_len, rhs.m_data, rhs.m_len);
            m_len += rhs.m_len;
        }
        return *this;
    }

    script_string script_string::operator +(const script_string& rhs) {
        script_string s(*this);
        s += rhs;
        return s;
    }

    script_string& script_string::operator +=(char ch) {
        if ((m_len + 1) >= m_capacity) resize(m_capacity + 32);
        
        m_data[m_len++] = ch;
        return *this;
    }

    script_string script_string::operator +(char ch) {
        script_string s(*this);
        s += ch;
        return s;
    }

    script_string& script_string::operator +=(i64 rhs) {
        resize(m_capacity + 32);
        snprintf(m_data + m_len, 32, "%lld", rhs);
        return *this;
    }
    
    script_string script_string::operator +(i64 rhs) {
        script_string s(*this);
        s += rhs;
        return s;
    }
    
    script_string& script_string::operator +=(u64 rhs) {
        resize(m_capacity + 32);
        snprintf(m_data + m_len, 32, "%llu", rhs);
        return *this;
    }
    
    script_string script_string::operator +(u64 rhs) {
        script_string s(*this);
        s += rhs;
        return s;
    }
    
    script_string& script_string::operator +=(f32 rhs) {
        resize(m_capacity + 32);
        snprintf(m_data + m_len, 32, "%f", rhs);
        return *this;
    }
    
    script_string script_string::operator +(f32 rhs) {
        script_string s(*this);
        s += rhs;
        return s;
    }
    
    script_string& script_string::operator +=(f64 rhs) {
        resize(m_capacity + 32);
        snprintf(m_data + m_len, 32, "%f", rhs);
        return *this;
    }

    script_string script_string::operator +(f64 rhs) {
        script_string s(*this);
        s += rhs;
        return s;
    }

    void script_string::resize(u64 cap) {
        m_capacity = cap;
        char* data = (char*)script_allocate(cap);
        if (m_len < m_capacity) memset(data + m_len, 0, m_capacity - m_len);
        if (m_len) memcpy(data, m_data, m_len);
        script_free(m_data);
        m_data = data;
    }
};