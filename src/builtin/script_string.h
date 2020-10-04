#pragma once
#include <common/types.h>
#include <string>

namespace gjs {
    class script_string {
        public:
            script_string();
            script_string(const std::string& str);
            script_string(const script_string& str);
            ~script_string();

            u32 length() const;
            char& operator[](u32 idx);

            inline const char* c_str() const { return m_data; }
            inline operator std::string() const { return std::string(m_data, m_len); }

            script_string& operator =(const std::string& rhs);
            script_string& operator =(const script_string& rhs);

        protected:
            char* m_data;
            u32 m_len;
            u32 m_capacity;
    };
};

