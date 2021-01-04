#pragma once
#include <string>
#include <gjs/common/types.h>
#include <gjs/util/robin_hood.h>

namespace gjs {
    class script_module;
    class script_enum {
        public:
            script_enum(const std::string& name);
            ~script_enum();

            std::string name() const;
            bool is_host() const;
            script_module* owner() const;

            void set(const std::string& value_name, i64 value);
            bool has(const std::string& value_name) const;
            i64 value(const std::string& value_name) const;

            const robin_hood::unordered_map<std::string, i64>& values() const {
                return m_values;
            }

        protected:
            friend class pipeline;
            friend class script_context;
            std::string m_name;
            bool m_is_host;
            script_module* m_owner;
            robin_hood::unordered_map<std::string, i64> m_values;
    };
};

