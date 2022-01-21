#pragma once
#include <gjs/common/types.h>
#include <gjs/vm/vm_state.h>
#include <gjs/vm/instruction_array.h>

namespace gjs {
    class vm_allocator;
    class vm_backend;
    class script_type;
    class script_function;
    struct raw_callback;
    class vm {
        public:
            vm(vm_backend* ctx, vm_allocator* alloc, u32 stack_size, u32 mem_size);
            ~vm();

            void execute(const instruction_array& code, address entry, bool nested);

            vm_allocator* alloc;
            vm_state state;

        protected:
            friend class script_function;
            void call_external(script_function* fn);
            void call_external(raw_callback** fn);
            
            vm_backend* m_ctx;
            u32 m_stack_size;

            // cached to prevent string comparisons for every function call
            script_type* m_subtype_t;
    };
};

