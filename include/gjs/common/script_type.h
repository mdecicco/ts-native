#pragma once
#include <gjs/common/types.h>
#include <gjs/common/errors.h>
#include <gjs/util/robin_hood.h>

#include <string>
#include <vector>

namespace gjs {
    namespace ffi {
        struct wrapped_class;
    };

    class script_module;
    class function_signature;

    enum class property_flags : u8 {
        pf_none             = 0b00000000,
        pf_read_only        = 0b00000001,
        pf_write_only       = 0b00000010,
        pf_pointer          = 0b00000100,
        pf_static           = 0b00001000
    };

    class script_function;
    class script_type {
        public:
            std::string name;
            std::string internal_name;
            u32 size;
            bool is_pod;
            bool is_primitive;
            bool is_unsigned;
            bool is_floating_point;
            bool is_builtin;
            bool is_host;
            bool is_trivially_copyable;
            bool requires_subtype;

            struct property {
                u8 flags;
                std::string name;
                script_type* type;
                u64 offset;
                script_function* getter;
                script_function* setter;
            };

            template <typename Ret, typename...Args>
            script_function* method(const std::string& _name) {
                return function_search<Ret, Args...>(owner->context(), name + "::" + _name, methods);
            }
            script_function* method(const std::string& name, script_type* ret, const std::vector<script_type*>& arg_types);

            property* prop(const std::string& name);

            script_module* owner;
            script_type* base_type;
            script_type* sub_type;
            std::vector<property> properties;
            script_function* destructor;
            std::vector<script_function*> methods;
            function_signature* signature;

            inline u32 id() const { return m_id; }

        protected:
            friend class type_manager;
            ffi::wrapped_class* m_wrapped;
            u32 m_id;

            script_type();
            ~script_type();
    };

    struct subtype_t {
        void* data;
    };
};