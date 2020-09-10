#pragma once
#include <common/types.h>
#include <string>
#include <vector>

namespace gjs {
    class script_context;
    class type_manager;
    class vm_type;
    enum class vm_register;

    namespace bind {
        struct wrapped_function;
    };

    class vm_function {
        public:
            vm_function(script_context* ctx, const std::string name, address addr);
            vm_function(type_manager* mgr, vm_type* tp, bind::wrapped_function* wrapped, bool is_ctor = false, bool is_dtor = false);

            void arg(vm_type* type);

            std::string name;
            bool is_host;
            bool is_static;
            vm_type* is_method_of;

            struct {
                vm_type* return_type;
                vm_register return_loc;
                std::vector<vm_type*> arg_types;
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

        protected:
            script_context* m_ctx;
    };
};

