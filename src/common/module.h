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
    class type_manager;

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
            std::enable_if_t<!std::is_class_v<T>, T> get_local(const std::string& name) {
                const local_var& v = local(name);
                T val;
                m_data->position(v.offset);
                m_data->read(val);
                return val;
            }

            template <typename T>
            std::enable_if_t<!std::is_class_v<T>, void> set_local(const std::string& name, const T& val) {
                const local_var& v = local(name);
                m_data->position(v.offset);
                m_data->write<T>(val);
            }

            bool has_function(const std::string& name);

            std::vector<script_function*> function_overloads(const std::string& name);

            /*
            * If a function is not overloaded, this function will return it by name only
            */
            script_function* function(const std::string& name);

            /*
            * If a function is overloaded, this function must be used
            */
            template <typename Ret, typename...Args>
            script_function* function(const std::string& name);

            std::vector<std::string> function_names() const;

            inline std::string name() const { return m_name; }
            inline u32 id() const { return m_id; }
            inline const std::vector<local_var>& locals() const { return m_locals; }
            inline const std::vector<script_function*>& functions() const { return m_functions; }
            inline type_manager* types() const { return m_types; }
            inline script_buffer* data() { return m_data; }

        protected:
            friend class pipeline;
            friend class script_context;
            script_module(script_context* ctx, const std::string& file);
            void add(script_function* func);

            std::string m_name;
            u32 m_id;
            script_function* m_init;
            std::vector<local_var> m_locals;


            std::vector<script_function*> m_functions;
            robin_hood::unordered_map<std::string, std::vector<u32>> m_func_map;
            robin_hood::unordered_map<std::string, u16> m_local_map;
            type_manager* m_types;
            script_context* m_ctx;
            script_buffer* m_data;
    };

    template <typename Ret, typename...Args>
    script_function* script_module::function(const std::string& name) {
        if (m_funcs.count(func->name) == 0) return nullptr;

        script_type* ret = m_types->get<Ret>();
        if (!ret) return nullptr;

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { m_types->get<Args>()... };

            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) return nullptr;
            }

            std::vector<script_function*>& funcs = m_funcs[func->name];
            for (u8 i = 0;i < funcs.size();i++) {
                bool matches = funcs[i]->signature.return_type->id() == ret->id();
                if (!matches) continue;

                if (funcs[i]->signature.arg_types.size() != arg_types.size()) continue;

                matches = true;
                for (u8 a = 0;a < funcs[i]->signature.arg_types.size() && matches;a++) {
                    matches = (funcs[i]->signature.arg_types[a]->id() == arg_types[a]->id());
                }

                if (matches) return funcs[i];
            }
        } else {
            std::vector<script_function*>& funcs = m_funcs[func->name];
            for (u8 i = 0;i < funcs.size();i++) {
                if (funcs[i]->signature.arg_types.size() != 0) continue;
                if (funcs[i]->signature.return_type->id() == ret->id()) return funcs[i];
            }
        }
        return nullptr;
    }
};