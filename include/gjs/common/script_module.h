#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <gjs/common/script_object.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/util/robin_hood.h>

#include <string>
#include <vector>

namespace gjs {
    class script_context;
    class script_function;
    class script_type;
    class script_buffer;
    class script_enum;
    class type_manager;
    class source_map;

    class script_module {
        public:
            struct local_var {
                u64 offset;
                script_type* type;
                std::string name;
                source_ref ref;
                script_module* owner;
                bool is_exported;
            };

            script_object init();

            // todo: tidy this mess up

            template <typename T>
            std::enable_if_t<!std::is_class_v<T>, T> get_local(const std::string& name) {
                const local_var& v = local_info(name);
                T val;
                m_data->position(v.offset);
                m_data->read(val);
                return val;
            }

            template <typename T>
            std::enable_if_t<!std::is_class_v<T>, void> set_local(const std::string& name, const T& val) {
                const local_var& v = local_info(name);
                m_data->position(v.offset);
                m_data->write<T>(val);
            }

            bool has_function(const std::string& name, bool include_private);

            std::vector<script_function*> function_overloads(const std::string& name, bool include_private);

            /*
            * If a function is not overloaded, this function will return it by name only.
            * If a function is overloaded, use function_search<Ret, Args...>(module, name)
            * from gjs/util/template_utils.h
            */
            script_function* function(const std::string& name);

            script_type* type(const std::string& name) const;
            std::vector<std::string> function_names() const;
            script_enum* get_enum(const std::string& name) const;
            script_object define_local(const std::string& name, script_type* type, bool is_exported);
            void define_local(const std::string& name, u64 offset, script_type* type, const source_ref& ref, bool is_exported);
            bool has_local(const std::string& name, bool include_private) const;
            const local_var& local_info(const std::string& name) const;
            void* local_ptr(const std::string& name) const;
            script_object local(const std::string& name) const;

            u64 add_trace(const source_ref& ref);

            inline std::string name() const { return m_name; }
            inline module_id id() const { return m_id; }
            inline const std::vector<local_var>& locals() const { return m_locals; }
            inline const std::vector<script_function*>& functions() const { return m_functions; }
            inline const std::vector<script_enum*>& enums() const { return m_enums; }
            inline type_manager* types() const { return m_types; }
            inline script_buffer* data() { return m_data; }
            inline script_context* context() const { return m_ctx; }
            inline source_map* trace_map() const { return m_traceMap; }

        protected:
            friend class pipeline;
            friend class script_context;
            friend class script_function;
            script_module(script_context* ctx, const std::string& name, const std::string& path);
            ~script_module();
            void add(script_function* func);

            std::string m_name;
            module_id m_id;
            script_function* m_init;
            std::vector<local_var> m_locals;
            std::vector<script_enum*> m_enums;
            std::vector<script_function*> m_functions;
            robin_hood::unordered_map<std::string, std::vector<u32>> m_func_map;
            robin_hood::unordered_map<std::string, u16> m_local_map;
            type_manager* m_types;
            script_context* m_ctx;
            script_buffer* m_data;
            source_map* m_traceMap;
            bool m_initialized;
    };
};
