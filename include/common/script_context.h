#pragma once
#include <bind/bind.h>
#include <common/source_map.h>
#include <util/util.h>
#include <common/pipeline.h>
#include <common/script_object.h>

#include <util/robin_hood.h>
#include <string>
#include <dyncall.h>


namespace gjs {
    class type_manager;
    class script_function;
    class script_module;
    //class script_object;
    class io_interface;
    class script_context {
        public:
            script_context(backend* generator = nullptr, io_interface* io = nullptr);
            ~script_context();

            template <class Cls>
            std::enable_if_t<std::is_class_v<Cls>, bind::wrap_class<Cls>&> bind(const std::string& name) {
                // as long as wrap_class::finalize is called, this will be deleted when it should be
                bind::wrap_class<Cls>* out = new bind::wrap_class<Cls>(m_global->types(), name);
                out->type->owner = m_global;
                return *out;
            }

            template <class prim>
            std::enable_if_t<!std::is_class_v<prim>, bind::pseudo_class<prim>&> bind(const std::string& name) {
                // as long as pseudo_class::finalize is called, this will be deleted when it should be
                bind::pseudo_class<prim>* out = new bind::pseudo_class<prim>(m_global->types(), name);
                out->type->owner = m_global;
                out->type->is_unsigned = std::is_unsigned_v<prim>;
                out->type->is_builtin = true;
                out->type->is_floating_point = std::is_floating_point_v<prim>;
                return *out;
            }


            template <typename Ret, typename... Args>
            script_function* bind(Ret(*func)(Args...), const std::string& name) {
                bind::wrapped_function* w = bind::wrap(m_global->types(), name, func);
                return new script_function(m_global->types(), nullptr, w);
            }

            script_module* module(const std::string& name);
            script_module* module(u32 id);
            std::vector<script_module*> modules() const;
            script_function* function(u64 address);

            void add(script_function* func);
            void add(script_module* module);

            inline script_module* global() { return m_global; }
            inline pipeline* compiler() { return &m_pipeline; }
            inline backend* generator() { return m_backend; }
            inline DCCallVM* call_vm() { return m_host_call_vm; }
            inline io_interface* io() { return m_io; }

            script_module* resolve(const std::string& module_path);
            script_module* resolve(const std::string& rel_path, const std::string& module_path);
            std::string module_name(const std::string& module_path);
            std::string module_name(const std::string& rel_path, const std::string& module_path);

            script_module* add_code(const std::string& module, const std::string& code);

            /*
            * Call function
            */
            template <typename Ret, typename... Args>
            void call(script_function* func, Ret* result, Args... args);

        protected:
            robin_hood::unordered_map<std::string, script_module*> m_modules;
            robin_hood::unordered_map<u32, script_module*> m_modules_by_id;
            robin_hood::unordered_map<u64, script_function*> m_funcs_by_addr;

            pipeline m_pipeline;
            backend* m_backend;
            DCCallVM* m_host_call_vm;
            script_module* m_global;
            io_interface* m_io;
            bool m_owns_backend;
            bool m_owns_io;
    };

    template <typename Ret, typename... Args>
    void script_context::call(script_function* func, Ret* result, Args... args) {
        // validate signature
        script_type* ret = nullptr;

        if constexpr (std::is_same_v<Ret, void>) ret = m_global->types()->get<void>();
        else ret = arg_type(this, result);

        if (!ret) {
            // exception
            return;
        }

        void* dest = (void*)result;
        if constexpr (std::is_same_v<remove_all<Ret>::type, script_object>) {
            if constexpr (std::is_pointer_v<Ret>) dest = (void*)&(*result)->m_self;
            else dest = (void*)&result->m_self;
        }

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = true;
        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(this, args)... };

            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                valid_call = func->signature.return_type->id() == ret->id();

                if (valid_call) {
                    if (func->signature.arg_types.size() != ac) valid_call = false;

                    for (u8 a = 0;a < func->signature.arg_types.size() && valid_call;a++) {
                        valid_call = (func->signature.arg_types[a]->id() == arg_types[a]->id());
                    }
                }
            }

            if (!valid_call) {
                // exception
                return;
            }

            void* vargs[] = { to_arg(args)... };
            m_backend->call(func, dest, vargs);
        } else {
            if (func->signature.arg_types.size() != 0) valid_call = false;
            else valid_call = (func->signature.return_type->id() == ret->id());

            if (!valid_call) {
                // exception
                return;
            }

            m_backend->call(func, dest, nullptr);
        }
    }
};
