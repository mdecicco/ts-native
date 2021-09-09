#pragma once
#include <gjs/bind/bind.h>
#include <gjs/util/util.h>
#include <gjs/common/source_map.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_object.h>
#include <gjs/common/function_pointer.h>

#include <gjs/util/robin_hood.h>
#include <string>


namespace gjs {
    class type_manager;
    class script_function;
    class script_module;
    class io_interface;
    class script_context {
        public:
            script_context(backend* generator = nullptr, io_interface* io = nullptr);
            ~script_context();

            template <class Cls>
            std::enable_if_t<std::is_class_v<Cls>, bind::wrap_class<Cls>&>
            bind(const std::string& name);

            template <class prim>
            std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, bind::pseudo_class<prim>&>
            bind(const std::string& name);

            template <typename Ret, typename... Args>
            script_function* bind(Ret(*func)(Args...), const std::string& name);

            template <typename... Args>
            script_object instantiate(script_type* type, Args... args);

            template <typename... Args>
            script_object construct_at(script_type* type, void* dest, Args... args);

            script_module* create_module(const std::string& name, const std::string& path);
            script_module* module(const std::string& name);
            script_module* module(u32 id);
            std::vector<script_module*> modules() const;
            const std::vector<script_function*>& functions() const;
            script_function* function(function_id id);

            void add(script_function* func);
            void add(script_module* module);

            inline script_module* global() { return m_global; }
            inline pipeline* compiler() { return &m_pipeline; }
            inline backend* generator() { return m_backend; }
            inline DCCallVM* call_vm() { return m_host_call_vm; }
            inline io_interface* io() { return m_io; }
            inline type_manager* types() { return m_all_types; }

            template <typename T>
            script_type* type(bool do_throw = false);

            script_module* resolve(const std::string& module_path);
            script_module* resolve(const std::string& rel_path, const std::string& module_path);
            std::string module_name(const std::string& module_path);
            std::string module_name(const std::string& rel_path, const std::string& module_path);

            script_module* add_code(const std::string& module_name, const std::string& module_path, const std::string& code);

            template <typename... Args>
            script_object call(script_function* func, void* self, Args... args);

            template <typename... Args>
            script_object call(u64 moduletype_id, script_function* func, void* self, Args... args);

            template <typename... Args>
            script_object call_callback(raw_callback* cb, Args... args);

            static script_context* current();

        protected:
            robin_hood::unordered_map<std::string, script_module*> m_modules;
            robin_hood::unordered_map<u32, script_module*> m_modules_by_id;
            std::vector<script_function*> m_funcs;
            type_manager* m_all_types;

            pipeline m_pipeline;
            backend* m_backend;
            DCCallVM* m_host_call_vm;
            script_module* m_global;
            io_interface* m_io;
            bool m_owns_backend;
            bool m_owns_io;
    };
};

#include <gjs/common/script_context.inl>