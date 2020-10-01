#pragma once
#include <bind/bind.h>
#include <common/source_map.h>
#include <util/util.h>
#include <common/pipeline.h>

#include <util/robin_hood.h>
#include <string>

namespace gjs {
    class type_manager;
    class script_function;
    class script_context {
        public:
            script_context(backend* generator);
            ~script_context();

            template <class Cls>
            bind::wrap_class<Cls>& bind(const std::string& name) {
                // as long as wrap_class::finalize is called, this will be deleted when it should be
                return *(new bind::wrap_class<Cls>(m_types, name));
            }

            template <typename Ret, typename... Args>
            script_function* bind(Ret(*func)(Args...), const std::string& name) {
                bind::wrapped_function* w = bind::wrap(m_types, name, func);
                return new script_function(m_types, nullptr, w);
            }

            /*
             * If a function is not overloaded, this function will return it by name only
             */
            script_function* function(const std::string& name);

            /*
             * If a function is overloaded, this function must be used
             */
            template <typename Ret, typename...Args>
            script_function* function(const std::string& name);

            script_function* function(u64 address);

            void add(script_function* func);

            std::vector<script_function*>   all_functions();
            std::vector<script_type*>       all_types();
            inline type_manager*        types     () { return m_types; }
            inline pipeline*            compiler  () { return &m_pipeline; }
            inline backend*             generator () { return m_backend; }

            bool add_code(const std::string& filename, const std::string& code);

            /*
            * Call function
            */
            template <typename Ret, typename... Args>
            void call(script_function* func, Ret* result, Args... args);

        protected:
            robin_hood::unordered_map<u64, script_function*> m_funcs_by_addr;
            robin_hood::unordered_map<std::string, std::vector<script_function*>> m_funcs;

            type_manager* m_types;
            pipeline m_pipeline;
            backend* m_backend;
    };

    template <typename Ret, typename...Args>
    script_function* script_context::function(const std::string& name) {
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

    template <typename Ret, typename... Args>
    void script_context::call(script_function* func, Ret* result, Args... args) {
        // validate signature
        script_type* ret = m_types->get<Ret>();
        if (!ret) {
            // exception
            return;
        }

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = true;
        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { m_types->get<Args>()... };

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
