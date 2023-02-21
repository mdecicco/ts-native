#pragma once
#include <tsn/common/types.h>

namespace tsn {
    class Context;
    void AddBuiltInBindings(Context* ctx);

    void* newMem(u64 size);
    void freeMem(void* mem);
    void copyMem(void* from, void* to, u64 size);
    void BindMemory(Context* ctx);
};