#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>

namespace utils {
    // TODO

    GeneralAllocator::GeneralAllocator() : IAllocator<void>() {
    }

    GeneralAllocator::~GeneralAllocator() {
    }

    size_t GeneralAllocator::totalCapacityBytes() {
        return UINT32_MAX;
    }

    size_t GeneralAllocator::maxAllocationSize() {
        return UINT32_MAX;
    }

    bool GeneralAllocator::isDynamicallySized() {
        return true;
    }

    void* GeneralAllocator::allocInternal(size_t count) {
        u8* ptr = (u8*)malloc(count + sizeof(size_t));
        if (!ptr) return nullptr;

        *(size_t*)ptr = count;
        void* ret = (void*)(ptr + sizeof(size_t));

        if (getPtrSize(ret) != count) {
            free(ptr);
            return nullptr;
        }

        return ret;
    }

    size_t GeneralAllocator::freeInternal(void* ptr) {
        void* base = (void*)(((u8*)ptr) - sizeof(size_t));
        size_t sz = *(size_t*)base;
        ::free(base);
        return sz;
    }

    size_t GeneralAllocator::getPtrSize(void* ptr) {
        void* base = (void*)(((u8*)ptr) - sizeof(size_t));
        return *(size_t*)base;
    }

    size_t GeneralAllocator::resetInternal() {
        return 0;
    }
};