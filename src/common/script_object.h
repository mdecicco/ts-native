#pragma once
#include <common/types.h>
#include <util/template_utils.hpp>

namespace gjs {
    class script_type;
    class script_function;
    class script_context;
    class script_module;

    class script_object {
        public:
            template <typename ...Args>
            script_object(script_context* ctx, script_type* type, Args... args) {
                m_ctx = ctx;
                m_type = type;
                m_mod = nullptr;
                m_self = nullptr;

                script_function* ctor = nullptr;

                constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
                if constexpr (ac > 0) {
                    ctor = m_type->method("constructor", type, { type, ctx->global()->types()->get<Args>(true)... });
                } else ctor = m_type->method("constructor", type, { type });

                m_self = new u8[type->size];
                if (ctor) {
                    ctx->call(ctor, this, this, args...);
                }
                // else throw error::runtime_exception(error::ecode::r_invalid_object_constructor, type->name.c_str());
            }
            ~script_object();

            template <typename Ret, typename... Args>
            void call(const std::string& func, Ret* result, Args... args) {
                func_signature sig = function_signature<Ret>(m_ctx, func, result, this, args...);
                script_function* f = m_type->method(func, sig.return_type, sig.arg_types);
                if (!f) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_method, m_type->name.c_str(), sig.to_string().c_str());
                }

                m_ctx->call<Ret>(f, result, this, args...);
            }

            template <typename T>
            T& prop(const std::string& name) {
                auto p = m_type->prop(name);
                if (!p) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_property, m_type->name.c_str(), name.c_str());
                }

                // todo: object properties, reference objects

                return *(T*)(m_self + p->offset);
            }

            template <typename T>
            const T& prop(const std::string& name) const {
                auto p = m_type->prop(name);
                if (!p) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_property, m_type->name.c_str(), name.c_str());
                }

                // todo: object properties, reference objects

                return *(T*)(m_self + p->offset);
            }

            inline void* self() const { return (void*)m_self; }
            inline script_type* type() const { return m_type; }
        protected:
            friend class script_context;
            script_context* m_ctx;
            script_type* m_type;
            script_module* m_mod;
            u8* m_self;
    };
};