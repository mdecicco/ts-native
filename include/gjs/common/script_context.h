#pragma once
#include <gjs/common/types.h>
#include <gjs/util/robin_hood.h>
#include <string>
#include <vector>

struct DCCallVM_;
typedef DCCallVM_ DCCallVM;

namespace gjs {
    class type_manager;
    class script_type;
    class script_function;
    class script_module;
    class io_interface;
    class pipeline;
    class backend;

    class script_context {
        public:
            script_context(backend* generator = nullptr, io_interface* io = nullptr);
            script_context(u32 argc, const char** argv, backend* generator = nullptr, io_interface* io = nullptr);
            ~script_context();

            script_module* create_module(const std::string& name, const std::string& path);
            void destroy_module(script_module* mod);
            script_module* module(const std::string& name);
            script_module* module(u32 id);
            std::vector<script_module*> modules() const;
            const std::vector<script_function*>& functions() const;
            script_function* function(function_id id);

            void add(script_function* func);
            void add(script_module* module);

            inline script_module* global() { return m_global; }
            inline pipeline* compiler() { return m_pipeline; }
            inline backend* generator() { return m_backend; }
            inline DCCallVM* call_vm() { return m_host_call_vm; }
            inline io_interface* io() { return m_io; }
            inline type_manager* types() { return m_all_types; }

            script_module* resolve(const std::string& module_path);
            script_module* resolve(const std::string& rel_path, const std::string& module_path);
            std::string module_name(const std::string& module_path);
            std::string module_name(const std::string& rel_path, const std::string& module_path);

            script_module* add_code(const std::string& module_name, const std::string& module_path, const std::string& code);

            static script_context* current();
            static script_type* type(u64 moduletype_id);

        protected:
            robin_hood::unordered_map<std::string, script_module*> m_modules;
            robin_hood::unordered_map<u32, script_module*> m_modules_by_id;
            std::vector<script_function*> m_funcs;
            type_manager* m_all_types;

            pipeline* m_pipeline;
            backend* m_backend;
            DCCallVM* m_host_call_vm;
            script_module* m_global;
            io_interface* m_io;
            bool m_owns_backend;
            bool m_owns_io;
    };
};
