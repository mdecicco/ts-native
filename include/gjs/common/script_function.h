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
            script_function(script_context* ctx, const std::string name, address addr, script_type* signature, script_type* method_of, script_module* mod = nullptr);
            script_function(type_manager* mgr, script_type* tp, bind::wrapped_function* wrapped, bool is_ctor = false, bool is_dtor = false, script_module* mod = nullptr);

            // should only be called from compiler for __init__ function with deferred signature assignment
            void update_signature(script_type* sig);

            template <typename... Args>
            script_object call(void* self, Args... args) {
                return m_ctx->call(this, self, args...);
            }

            template <typename... Args>
            script_object call(u64 moduletype_id, void* self, Args... args) {
                return m_ctx->call(moduletype_id, this, self, args...);
            }

            std::string name;
            bool is_host;
            bool is_static;
            bool is_thiscall;
            bool is_subtype_obj_ctor;
            script_type* is_method_of;
            script_type* type;

            union {
                bind::wrapped_function* wrapped;
                address entry;
            } access;

            script_context* ctx() const;
            u32 id() const;

            script_module* owner;
        protected:
            script_context* m_ctx;
            u32 m_id;
    };
};

