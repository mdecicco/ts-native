#pragma once
#include <gjs/bind/bind.h>
#include <gjs/common/source_map.h>
#include <gjs/util/util.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_object.h>

#include <gjs/util/robin_hood.h>
#include <string>
#include <dyncall.h>


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
            bind(const std::string& name) {
                return m_global->bind<Cls>(name);
            }

            template <class prim>
            std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, bind::pseudo_class<prim>&>
            bind(const std::string& name) {
                return m_global->bind<prim>(name);
            }

            template <typename Ret, typename... Args>
            script_function* bind(Ret(*func)(Args...), const std::string& name) {
                return m_global->bind<Ret, Args...>(func, name);
            }

            template <typename... Args>
            script_object instantiate(script_type* type, Args... args) {
                return script_object(this, type, args...);
            }

            template <typename... Args>
            script_object construct_at(script_type* type, void* dest, Args... args) {
                return script_object(this, type, (u8*)dest, args...);
            }

            script_module* create_module(const std::string& name);
            script_module* module(const std::string& name);
            script_module* module(u32 id);
            std::vector<script_module*> modules() const;
            std::vector<script_function*> functions() const;
            script_function* function(u64 address);

            void add(script_function* func);
            void add(script_module* module);

            inline script_module* global() { return m_global; }
            inline pipeline* compiler() { return &m_pipeline; }
            inline backend* generator() { return m_backend; }
            inline DCCallVM* call_vm() { return m_host_call_vm; }
            inline io_interface* io() { return m_io; }
            inline type_manager* types() { return m_all_types; }

            template <typename T>
            script_type* type(bool do_throw = false) {
                return m_all_types->get<T>(do_throw);
            }

            script_module* resolve(const std::string& module_path);
            script_module* resolve(const std::string& rel_path, const std::string& module_path);
            std::string module_name(const std::string& module_path);
            std::string module_name(const std::string& rel_path, const std::string& module_path);

            script_module* add_code(const std::string& module, const std::string& code);
            
            template <typename... Args>
            script_object call(script_function* func, void* self, Args... args);

        protected:
            robin_hood::unordered_map<std::string, script_module*> m_modules;
            robin_hood::unordered_map<u32, script_module*> m_modules_by_id;
            robin_hood::unordered_map<u64, script_function*> m_funcs_by_addr;
            type_manager* m_all_types;

            pipeline m_pipeline;
            backend* m_backend;
            DCCallVM* m_host_call_vm;
            script_module* m_global;
            io_interface* m_io;
            bool m_owns_backend;
            bool m_owns_io;
    };

    template <typename... Args>
    script_object script_context::call(script_function* func, void* self, Args... args) {
        // validate signature
        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = (self != nullptr) == func->signature.is_thiscall;

        if (!valid_call) {
            // todo
            if (self) {
                // self for non-thiscall exception
            } else {
                // no self for thiscall exception
            }
            return script_object(this);
        }

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(this, args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->signature.arg_types.size() != ac) valid_call = false;

                for (u8 a = 0;a < func->signature.arg_types.size() && valid_call;a++) {
                    valid_call = (func->signature.arg_types[a]->id() == arg_types[a]->id());
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(this);
            }

            std::vector<void*> vargs = { to_arg(args)... };
            if (self) vargs.insert(vargs.begin(), self);

            script_object out = script_object(this, (script_module*)nullptr, func->signature.return_type, nullptr);
            if (func->signature.return_type->size > 0) {
                out.m_self = new u8[func->signature.return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, vargs.data());
            return out;
        } else {
            if (func->signature.arg_types.size() != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(this);
            }

            script_object out = script_object(this, (script_module*)nullptr, func->signature.return_type, nullptr);
            if (func->signature.return_type->size > 0) {
                out.m_self = new u8[func->signature.return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, &self);
            return out;
        }

        return script_object(this);
    }
};
