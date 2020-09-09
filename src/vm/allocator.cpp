#include <vm/allocator.h>
#include <stdlib.h>

namespace gjs {
    void* basic_malloc_allocator::allocate(u32 size) {
        return malloc(size);
    }

    void* basic_malloc_allocator::reallocate(void* ptr, u32 size) {
        return realloc(ptr, size);
    }

    void basic_malloc_allocator::deallocate(void* ptr) {
        free(ptr);
    }
};