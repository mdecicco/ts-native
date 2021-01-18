#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_type.h>
#include <gjs/util/template_utils.hpp>

namespace gjs {
    class script_function;
    class script_context;
    class script_module;
    class script_object;
    struct property_accessor;

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
                m_propInfo = nullptr;
                m_refCount = new u32(1);

                script_function* ctor = nullptr;

                constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
                if constexpr (ac > 0) {
                    ctor = m_type->method("constructor", nullptr, { ctx->global()->types()->get<Args>(true)... });
                } else ctor = m_type->method("constructor", nullptr, {});

                m_self = new u8[type->size];
                if (ctor) ctor->call(this, args...);
                else throw error::runtime_exception(error::ecode::r_invalid_object_constructor, type->name.c_str());
            }
            script_object(const script_object& o);
            ~script_object();

            template <typename Ret = void, typename... Args>
            script_object call(const std::string& func, Args... args) {
                if (!m_self) {
                    // todo runtime exception
                    return script_object(m_ctx);
                }

                func_signature sig = function_signature<Ret>(m_ctx, func, (Ret*)nullptr, args...);
                script_function* f = m_type->method(func, sig.return_type, sig.arg_types);
                if (!f) {
                    throw error::runtime_exception(error::ecode::r_invalid_object_method, m_type->name.c_str(), sig.to_string().c_str());
                    return script_object(m_ctx);
                }

                return f->call(this, args...);
            }

            script_object operator [] (const std::string& name);

            template <typename T>
            script_object operator = (const T& rhs) {
                using base_tp = remove_all<T>::type;
                if constexpr (std::is_same_v<T, script_object>) assign(rhs);
                else if constexpr (std::is_same_v<T, script_object*>) assign(*rhs);
                else {
                    if (std::is_trivially_copyable_v<base_tp> && std::is_pod_v<base_tp> && m_type->size == sizeof(base_tp) && m_type->is_trivially_copyable && m_type->is_pod) {
                        script_type* tp = m_ctx->type<T>(false);
                        if (tp && !tp->is_primitive) {
                            // favor copy construction for structures
                            if (assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)&rhs))) return *this;
                        }

                        // assumes that T is an unbound type with the same layout as this object
                        if constexpr (std::is_pointer_v<T>) *(base_tp*)m_self = *rhs;
                        else *(base_tp*)m_self = rhs;
                    } else {
                        if constexpr (std::is_pointer_v<T>) {
                            script_type* tp = m_ctx->type<T>(true);
                            assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)rhs));
                        } else {
                            script_type* tp = m_ctx->type<T>(true);
                            assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)&rhs));
                        }
                    }
                }

                return *this;
            }

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
            friend struct property_accessor;
            script_object(script_context* ctx); // undefined value
            script_object(const property_accessor& prop);
            script_object(script_context* ctx, script_module* mod, script_type* type, u8* ptr);
            bool assign(const script_object& rhs);

            bool m_destructed;
            bool m_owns_ptr;
            script_context* m_ctx;
            script_type* m_type;
            script_module* m_mod;
            u8* m_self;
            property_accessor* m_propInfo;
            u32* m_refCount;
    };

    struct property_accessor {
        script_object obj;
        script_type::property* prop;

        script_object get() const;
        script_object set(const script_object& rhs);
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