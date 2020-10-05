#pragma once
#include <common/types.h>
#include <string>

namespace gjs {
    class script_string {
        public:
            script_string();
            script_string(const std::string& str);
            script_string(const script_string& str);
            script_string(char* str, u32 len);
            script_string(void* str, u64 len) : script_string((char*)str, (u32)len) { }
            ~script_string();

            u32 length() const;
            char& operator[](u32 idx);

            inline const char* c_str() const { return m_data; }
            inline operator std::string() const { return std::string(m_data, m_len); }
            inline operator void*() const { return m_data; }

            script_string& operator =(const std::string& rhs);
            script_string& operator =(const script_string& rhs);
            script_string& operator +=(const script_string& rhs);
            script_string operator +(const script_string& rhs);
            script_string& operator +=(char ch);
            script_string operator +(char ch);
            script_string& operator +=(i64 rhs);
            script_string operator +(i64 rhs);
            script_string& operator +=(u64 rhs);
            script_string operator +(u64 rhs);
            script_string& operator +=(f32 rhs);
            script_string operator +(f32 rhs);
            script_string& operator +=(f64 rhs);
            script_string operator +(f64 rhs);

        protected:
            void resize(u32 cap);
            char* m_data;
            u32 m_len;
            u32 m_capacity;
    };
};

