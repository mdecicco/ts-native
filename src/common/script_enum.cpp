#include <gjs/common/script_enum.h>
#include <gjs/bind/bind.h>
#include <gjs/util/util.h>
#include <gjs/common/errors.h>

namespace gjs {
    script_enum::script_enum(const std::string& name) {
        m_name = name;
        m_is_host = false;
        m_owner = nullptr;
    }

    script_enum::~script_enum() {
    }

    std::string script_enum::name() const {
        return m_name;
    }

    bool script_enum::is_host() const {
        return m_is_host;
    }

    script_module* script_enum::owner() const {
        return m_owner;
    }

    void script_enum::set(const std::string& value_name, i64 value) {
        auto it = m_values.find(value_name);
        if (it != m_values.end()) {
            throw error::bind_exception(error::ecode::b_enum_value_exists, value_name.c_str(), m_name.c_str());
        }

        m_values[value_name] = value;
    }

    bool script_enum::has(const std::string& value_name) const {
        return m_values.count(value_name);
    }

    i64 script_enum::value(const std::string& value_name) const {
        auto it = m_values.find(value_name);
        if (it == m_values.end()) {
            throw error::runtime_exception(error::ecode::r_invalid_enum_value, m_name.c_str(), value_name.c_str());
        }

        return it->second;
    }
};