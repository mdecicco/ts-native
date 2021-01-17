#pragma once
#include <common/types.h>
#include <common/script_type.h>
#include <util/template_utils.hpp>

namespace gjs {
    class script_function;
    class script_context;
    class script_module;
    class script_object;
    
    template <typename T>
    struct property_accessor {
        const script_object* obj;
        script_type::property* prop;

        inline T& ref();
        inline T get() const;
        inline T set(const T& rhs);

        inline operator T&() { return ref(); }
        inline operator T() const { return get(); }
        inline T operator = (const T& rhs) { return set(rhs); }
    };

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
            property_accessor<T> prop(const std::string& name) const {
                script_type::property* p = m_type->prop(name);
                if (!p) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_property, m_type->name.c_str(), name.c_str());
                }

                return { this, p };
            }

            inline void* self() const { return (void*)m_self; }
            inline script_type* type() const { return m_type; }
            inline script_context* context() const { return m_ctx; }
        protected:
            friend class script_context;
            script_context* m_ctx;
            script_type* m_type;
            script_module* m_mod;
            u8* m_self;
    };

    template <typename T>
    inline T& property_accessor<T>::ref() {
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
    inline T property_accessor<T>::get() const {
        // todo: objects

        if (!(prop->flags & bind::pf_write_only) && !prop->getter && !prop->setter) {
            return *(T*)(((u8*)obj->self()) + prop->offset);
        }

        if (prop->getter) {
            u8 r[sizeof(T)];
            obj->context()->call<T>(prop->getter, (T*)r, obj);
            return *(T*)r;
        }

        throw error::runtime_exception(error::ecode::r_cannot_read_object_property, prop->name.c_str(), obj->type()->name.c_str());
    }

    template <typename T>
    inline T property_accessor<T>::set(const T& rhs) {
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
};