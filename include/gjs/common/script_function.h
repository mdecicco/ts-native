#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <string>
#include <vector>

namespace gjs {
    class script_context;
    class type_manager;
    class script_type;
    class script_module;
    enum class vm_register;

    namespace ffi {
        struct wrapped_function;
    };

    class script_function {
        public:
            script_function(script_context* ctx, const std::string name, address addr, script_type* signature, script_type* method_of, script_module* mod = nullptr);
            script_function(type_manager* mgr, script_type* tp, ffi::wrapped_function* wrapped, bool is_ctor = false, bool is_dtor = false, script_module* mod = nullptr);

            script_function* duplicate_with_subtype(script_type* subtype);

            std::string name;
            bool is_external;
            bool is_exported;
            bool is_host;
            bool is_static;
            bool is_thiscall;
            bool is_subtype_obj_ctor;
            source_ref src;
            script_type* is_method_of;
            script_type* type;

            union {
                ffi::wrapped_function* wrapped;
                address entry;
            } access;

            script_context* ctx() const;
            function_id id() const;
            bool is_clone() const;

            script_module* owner;
        protected:
            friend class script_context;
            friend class pipeline;
            script_context* m_ctx;
            bool m_is_copy;
            function_id m_id;
    };
};

