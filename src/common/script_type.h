#pragma once
#include <util/template_utils.hpp>
#include <common/types.h>

#include <string>
#include <vector>
#include <util/robin_hood.h>

namespace gjs {
    namespace bind {
        struct wrapped_class;
    };

    class script_context;
    class script_type;

    class type_manager {
        public:
            type_manager(script_context* ctx);
            ~type_manager();

            script_type* get(const std::string& internal_name);

            script_type* get(u32 id);

            template <typename T>
            script_type* get() {
                return get(base_type_name<T>());
            }

            script_type* add(const std::string& name, const std::string& internal_name);

            script_type* finalize_class(bind::wrapped_class* wrapped);

            std::vector<script_type*> all();

        protected:
            friend class script_function;
            script_context* m_ctx;
            robin_hood::unordered_map<std::string, script_type*> m_types;
            robin_hood::unordered_map<u32, script_type*> m_types_by_id;
    };

    class script_function;
    class script_type {
        public:
            std::string name;
            std::string internal_name;
            u32 size;
            bool is_primitive;
            bool is_unsigned;
            bool is_floating_point;
            bool is_builtin;
            bool is_host;
            bool requires_subtype;

            struct property {
                u8 flags;
                std::string name;
                script_type* type;
                u64 offset;
                script_function* getter;
                script_function* setter;
            };

            script_type* base_type;
            script_type* sub_type;
            std::vector<property> properties;
            script_function* destructor;
            std::vector<script_function*> methods;

            inline u32 id() const { return m_id; }

        protected:
            friend class type_manager;
            bind::wrapped_class* m_wrapped;
            u32 m_id;
            script_type();
            ~script_type();
    };

    struct subtype_t {
        void* data;
    };
};