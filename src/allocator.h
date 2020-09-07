#pragma once
#include <types.h>

namespace gjs {
    class vm_allocator {
        public:
            vm_allocator() { }
            ~vm_allocator() { }

            virtual void* allocate(u32 size) = 0;
            virtual void* reallocate(void* ptr, u32 size) = 0;
            virtual void deallocate(void* ptr) = 0;
    };

    class basic_malloc_allocator : public vm_allocator {
        public:
            basic_malloc_allocator() { }
            ~basic_malloc_allocator() { }

            virtual void* allocate(u32 size);
            virtual void* reallocate(void* ptr, u32 size);
            virtual void deallocate(void* ptr);
    };
};