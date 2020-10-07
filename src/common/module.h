#pragma once
#include <common/types.h>
#include <common/source_ref.h>
#include <util/robin_hood.h>
#include <string>
#include <vector>

namespace gjs {
    class script_context;
    class script_function;
    class script_type;
    class script_buffer;

    class script_module {
        public:
            struct local_var {
                u64 offset;
                script_type* type;
                std::string name;
                source_ref ref;
            };
            ~script_module();

            void init();

            void define_local(const std::string& name, u64 offset, script_type* type, const source_ref& ref);
            bool has_local(const std::string& name) const;
            const local_var& local(const std::string& name) const;

            template <typename T>
            std::enable_if_t<!std::is_class_v<T>, T> local_value(const std::string& name) {
                const local_var& v = local(name);
                T val;
                m_data->position(v.offset);
                m_data->read(val);
                return val;
            }

            inline std::string name() const { return m_name; }
            inline u32 id() const { return m_id; }
            inline const std::vector<local_var>& locals() const { return m_locals; }
            inline const std::vector<script_function*>& functions() const { return m_functions; }
            inline const std::vector<script_type*>& types() const { return m_types; }
            inline script_buffer* data() { return m_data; }

        protected:
            friend class pipeline;
            script_module(script_context* ctx, const std::string& file);

            std::string m_name;
            u32 m_id;
            script_function* m_init;
            std::vector<script_function*> m_functions;
            std::vector<script_type*> m_types;
            std::vector<local_var> m_locals;
            robin_hood::unordered_map<std::string, u16> m_local_map;
            script_context* m_ctx;
            script_buffer* m_data;
    };
};