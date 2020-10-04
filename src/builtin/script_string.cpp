#include <builtin/script_string.h>

namespace gjs {
    script_string::script_string() : m_data(nullptr), m_len(0), m_capacity(0) {
    }

    script_string::script_string(const std::string& str) : m_data(nullptr), m_len(0), m_capacity(0) {
    }

    script_string::script_string(const script_string& str) : m_data(nullptr), m_len(0), m_capacity(0) {
    }

    script_string::~script_string() {
    }

    u32 script_string::length() const {
        return m_len;
    }

    char& script_string::operator[](u32 idx) {
        return m_data[idx];
    }

    script_string& script_string::operator =(const std::string& rhs) {
        return *this;
    }

    script_string& script_string::operator =(const script_string& rhs) {
        return *this;
    }
};