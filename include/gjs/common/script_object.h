#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_type.h>
#include <gjs/util/template_utils.hpp>

namespace gjs {
    class script_function;
    class script_context;
    class script_module;
    class script_object;
    
    struct property_accessor {
        const script_object* obj;
        script_type::property* prop;

        operator script_object();
        script_object operator = (const script_object& rhs);
    };

    class script_object {
        public:
            template <typename ...Args>
            script_object(script_context* ctx, script_type* type, Args... args) {
                m_ctx = ctx;
                m_type = type;
                m_mod = nullptr;
                m_self = nullptr;
                m_owns_ptr = true;
                m_destructed = false;

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
                if (!m_self) {
                    // todo runtime exception
                    return;
                }

                func_signature sig = function_signature<Ret>(m_ctx, func, result, this, args...);
                script_function* f = m_type->method(func, sig.return_type, sig.arg_types);
                if (!f) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_method, m_type->name.c_str(), sig.to_string().c_str());
                }

                m_ctx->call<Ret>(f, result, this, args...);
            }

            property_accessor prop(const std::string& name) const;

            bool is_null() const;

            template <typename T>
            operator T&() {
                return *(T*)m_self;
            }

            template <typename T>
            operator T*() {
                return (T*)m_self;
            }

            inline void* self() const { return (void*)m_self; }
            inline script_type* type() const { return m_type; }
            inline script_context* context() const { return m_ctx; }
        protected:
            friend class script_context;
            friend class script_module;
            friend class property_accessor;
            script_object(script_context* ctx); // undefined value
            script_object(script_context* ctx, script_module* mod, script_type* type, u8* ptr);
            void destruct();

            bool m_destructed;
            bool m_owns_ptr;
            script_context* m_ctx;
            script_type* m_type;
            script_module* m_mod;
            u8* m_self;
    };
    /*
    template <typename T>
    inline T& property_accessor::ref() {
        // todo: objects

        if (prop->flags & bind::pf_read_only || prop->flags & bind::pf_write_only) {
            throw error::runtime_exception(error::ecode::r_cannot_ref_restricted_object_property, prop->name.c_str(), obj->type()->name.c_str());
        }

        if (prop->getter || prop->setter) {
            throw error::runtime_exception(error::ecode::r_cannot_ref_object_accessor_property, prop->name.c_str(), obj->type()->name.c_str());
        }

        return *(T*)(((u8*)obj->self()) + prop->offset);
    }

    template <typename T>
    inline T property_accessor::get() const {
    }

    template <typename T>
    inline T property_accessor::set(const T& rhs) {
        if (!(prop->flags & bind::pf_read_only) && !prop->getter && !prop->setter) {
            return (*(T*)(((u8*)obj->self()) + prop->offset)) = rhs;
        }

        if (prop->setter) {
            u8 r[sizeof(T)];
            obj->context()->call<T>(prop->setter, (T*)r, obj, rhs);
            return *(T*)r;
        }

        throw error::runtime_exception(error::ecode::r_cannot_write_object_property, prop->name.c_str(), obj->type()->name.c_str());
    }
    */
};