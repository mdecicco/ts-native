#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/bind/bind.hpp>
#include <utils/Allocator.hpp>

using namespace tsn::ffi;

namespace tsn {
    void* newMem(u64 size) {
        return utils::Mem::alloc(size);
    }

    void freeMem(void* mem) {
        utils::Mem::free(mem);
    }

    void copyMem(void* from, void* to, u64 size) {
        memcpy(to, from, size);
    }

    void BindMemory(Context* ctx) {
        Module* m = ctx->createModule("memory");
        bind(m, "newMem", newMem, trusted_access);
        bind(m, "freeMem", freeMem, trusted_access);
        bind(m, "copyMem", copyMem, trusted_access);
    }
};