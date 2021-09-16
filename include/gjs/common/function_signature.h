#pragma once
#include <gjs/common/types.h>

#include <vector>
#include <string>

namespace gjs {
    class script_context;
    class script_type;
    class script_module;
    class type_manager;
    enum class vm_register;


    namespace ffi {
        struct wrapped_function;
    };

    class function_signature {
        public:
            struct argument {
                enum class implicit_type : u8 {
                    not_implicit = 0,
                    capture_data_ptr,
                    this_ptr,
                    moduletype_id,
                    ret_addr
                };

                script_type* tp;
                vm_register loc;
                implicit_type implicit;
                bool is_ptr;
            };

            function_signature(script_context* ctx, script_type* tp, ffi::wrapped_function* wrapped, bool is_ctor = false);
            function_signature(script_context* ctx, script_type* ret, bool ret_ptr, script_type** args, u8 argc, script_type* method_of, bool is_ctor = false, bool is_static_method = false, bool is_callback = false);
            function_signature();

            const argument& explicit_arg(u8 idx) const;

            std::string to_string() const;
            std::string to_string(const std::string& funcName, script_type* method_of = nullptr, script_module* mod = nullptr) const;

            script_type* method_of;
            script_type* return_type;
            vm_register return_loc;
            bool is_thiscall;
            bool returns_on_stack;
            bool returns_pointer;
            u8 implicit_argc;
            u8 explicit_argc;
            std::vector<argument> args;
    };
};
