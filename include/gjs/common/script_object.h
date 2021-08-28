#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_type.h>
#include <gjs/common/function_signature.h>

namespace gjs {
    class script_function;
    class script_context;
    class script_module;
    class script_object;
    struct property_accessor;

    class script_object {
        public:
            template <typename ...Args>
            script_object(script_context* ctx, script_type* type, Args... args);
            template <typename ...Args>
            script_object(script_context* ctx, script_type* type, u8* ptr, Args... args);
            script_object(script_type* type, u8* ptr);
            script_object(const script_object& o);
            ~script_object();

            template <typename Ret = void, typename... Args>
            script_object call(const std::string& func, Args... args);

            script_object operator [] (const std::string& name);

            template <typename T>
            script_object operator = (const T& rhs);

            bool is_null() const;

            // todo: non-trivial casts

            template <typename T>
            operator T&();

            template <typename T>
            operator T*();

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
    };
};

#include <gjs/common/script_object.inl>