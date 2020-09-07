#pragma once
#include <types.h>

namespace gjs {
    class vm_context;
    class instruction_array;
    class source_map;

    // Removes instructions for restoring registers (other than $ra and return register)
    // from the stack after a function call when the function returns immediately after
    void ir_remove_trailing_stack_loads(vm_context* ctx, instruction_array& ir, source_map* map, u32 start_addr);
};