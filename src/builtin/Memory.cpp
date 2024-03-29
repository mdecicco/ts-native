#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Closure.h>
#include <tsn/bind/bind.hpp>
#include <utils/Allocator.hpp>

using namespace tsn::ffi;

namespace tsn {
    struct capture_data {
        u8 data[1024];
    };

    utils::PagedAllocator<CaptureData> closureAllocator([](){
        return new utils::FixedAllocator<CaptureData>(128, 128);
    });

    utils::PagedAllocator<capture_data> captureDataAllocator([](){
        return new utils::FixedAllocator<capture_data>(256, 256);
    });

    CaptureData* newCaptureData(function_id targetId, void* captureData) {
        CaptureData* c = closureAllocator.alloc(1);
        new (c) CaptureData(ffi::ExecutionContext::Get(), targetId, captureData);
        return c;
    }

    void freeClosure(CaptureData* closure) {
        closure->~CaptureData();
        closureAllocator.free(closure);
    }

    void* allocCaptures(u32 captureSize, u32 captureCount) {
        u32 totalSize = captureSize + (captureCount * sizeof(type_id)) + sizeof(u32);
        u32 rem = totalSize % sizeof(capture_data);
        if (rem != 0) totalSize += (sizeof(capture_data) - rem);
        return captureDataAllocator.alloc(totalSize / sizeof(capture_data));;
    }

    void freeCaptureData(void* data) {
        captureDataAllocator.free((capture_data*)data);
    }

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
        Module* m = ctx->createHostModule("memory");
        bind(m, "$newCaptureData", newCaptureData, trusted_access);
        bind(m, "$allocCaptures", allocCaptures, trusted_access);
        bind(m, "newMem", newMem, trusted_access);
        bind(m, "freeMem", freeMem, trusted_access);
        bind(m, "copyMem", copyMem, trusted_access);
    }
};