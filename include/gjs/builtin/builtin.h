#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_context;

    void set_builtin_context(script_context* ctx);
    void init_context(script_context* ctx);

    // Gets the context of the current thread, if one is currently executing
    script_context* current_ctx();
    
    void* script_allocate(u64 size);
    void script_free(void* ptr);
    void script_copymem(void* to, void* from, u64 size);
};