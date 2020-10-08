#pragma once
#include <bind/bind.h>
#include <common/source_map.h>
#include <util/util.h>
#include <common/pipeline.h>

#include <util/robin_hood.h>
#include <string>
#include <dyncall.h>


namespace gjs {
    class type_manager;
    class script_function;
    class script_module;
    class script_context {
        public:
            script_context(backend* generator);
            ~script_context();

            template <class Cls>
            bind::wrap_class<Cls>& bind(const std::string& name) {
                // as long as wrap_class::finalize is called, this will be deleted when it should be
                bind::wrap_class<Cls>* out = new bind::wrap_class<Cls>(m_global->types(), name);
                out->type->owner = m_global;
                return *out;
            }

            template <typename Ret, typename... Args>
            script_function* bind(Ret(*func)(Args...), const std::string& name) {
                bind::wrapped_function* w = bind::wrap(m_global->types(), name, func);
                return new script_function(m_global->types(), nullptr, w);
            }

            script_module* module(const std::string& name);
            script_module* module(u32 id);
            script_function* function(u64 address);

            void add(script_function* func);
            void add(script_module* module);

            inline script_module* global() { return m_global; }
            inline pipeline* compiler() { return &m_pipeline; }
            inline backend* generator() { return m_backend; }
            inline DCCallVM* call_vm() { return m_host_call_vm; }

            script_module* resolve(const std::string& rel_path, const std::string& module_path);

            script_module* add_code(const std::string& filename, const std::string& code);

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
    };

    template <typename Ret, typename... Args>
    void script_context::call(script_function* func, Ret* result, Args... args) {
        // validate signature
        script_type* ret = m_global->types()->get<Ret>();
        if (!ret) {
            // exception
            return;
        }

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = true;
        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { m_global->types()->get<Args>()... };

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

            void* vargs[] = {
                std::is_class_v<Args> ?
                // Pass non-pointer struct/class values as pointers to those values
                // The only way that functions accepting these types could be bound
                // is if the argument type is a pointer or a reference (which is
                // basically a pointer)
                (void*)&args
                : 
                // Otherwise, cast the value to void*
                *reinterpret_cast<void**>(&args)...
            };
            m_backend->call(func, (void*)result, vargs);
        } else {
            if (func->signature.arg_types.size() != 0) valid_call = false;
            else valid_call = (func->signature.return_type->id() == ret->id());

            if (!valid_call) {
                // exception
                return;
            }

            m_backend->call(func, (void*)result, nullptr);
        }
    }
};
