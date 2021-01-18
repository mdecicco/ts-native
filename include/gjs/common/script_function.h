#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_object.h>
#include <string>
#include <vector>

namespace gjs {
    class script_context;
    class type_manager;
    class script_type;
    class script_module;
    enum class vm_register;

    namespace bind {
        struct wrapped_function;
    };

    class script_function {
        public:
            script_function(script_context* ctx, const std::string name, address addr);
            script_function(type_manager* mgr, script_type* tp, bind::wrapped_function* wrapped, bool is_ctor = false, bool is_dtor = false);

            void arg(script_type* type);

            template <typename... Args>
            script_object call(void* self, Args... args) {
                return m_ctx->call(this, self, args...);
            }

            std::string name;
            bool is_host;
            bool is_static;
            script_type* is_method_of;

            struct {
                script_type* return_type;
                vm_register return_loc;
                std::vector<script_type*> arg_types;
                std::vector<vm_register> arg_locs;
                bool returns_on_stack;
                bool is_thiscall;
                bool returns_pointer;
                bool is_subtype_obj_ctor;
            } signature;

            union {
                bind::wrapped_function* wrapped;
                address entry;
            } access;

            script_module* owner;
        protected:
            script_context* m_ctx;
    };
};

