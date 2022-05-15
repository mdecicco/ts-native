#pragma once
#include <utils/Types.h>

#include <utils/Singleton.h>
#include <utils/robin_hood.h>

#include <functional>

namespace utils {
    template <typename T, size_t ElemSz = sizeof(std::conditional_t<std::is_void_v<T>, u8, T>)>
    class IAllocator {
        public:
            IAllocator();
            virtual ~IAllocator();

            T* alloc(size_t count = 1);

            T* realloc(T* ptr, size_t count);

            size_t free(T* ptr);

            size_t reset();

            void copy(T* to, const T* from, size_t count);

            size_t elemSize() const;

            size_t memUsed() const;

            virtual size_t totalCapacityBytes() = 0;

            virtual size_t maxAllocationSize() = 0;

            virtual bool isDynamicallySized() = 0;

            virtual size_t getPtrSize(void* ptr) = 0;

        protected:
            static size_t ElementSize;
            virtual void* allocInternal(size_t count) = 0;
            virtual size_t freeInternal(void* ptr) = 0;
            virtual size_t resetInternal() = 0;

        private:
            size_t m_memUsed;
    };

    template <typename T>
    class FixedAllocator : public IAllocator<T> {
        private:
            struct elem_node {
                bool used;
                u8* ptr;
                size_t count;
                elem_node* next;
                elem_node* last;
            };

        public:
            /*
             * @param elementCapacity
             *     Total number of elements of type T that can be allocated.
             * @param maxLiveAllocations
             *     Total number of concurrent allocations allowed. (default = elementCapacity)
             * @param zeroMem
             *     Whether or not to initialize memory that gets allocated to zero. (default = false)
             */
            FixedAllocator(size_t elementCapacity, size_t maxLiveAllocations = 0, bool zeroMem = false);

            ~FixedAllocator();

            virtual size_t totalCapacityBytes();

            virtual size_t maxAllocationSize();

            virtual bool isDynamicallySized();

            virtual size_t getPtrSize(void* ptr);

        protected:
            virtual void* allocInternal(size_t count);

            virtual size_t freeInternal(void* ptr);

            virtual size_t resetInternal();

            elem_node* getEmptyNode();

            void releaseNode(elem_node* node);

            bool m_doZeroMem;
            size_t m_byteCapacity;
            size_t m_elementCapacity;
            u8* m_memBase;
            u8* m_nodeBase;

            // Hash map of pointer -> node
            robin_hood::unordered_map<void*, elem_node*> m_liveMap;

            // Linked list of memory blocks
            elem_node* m_memList;

            // Linked list of available nodes
            elem_node* m_freeList;
    };

    template <typename T, typename PageAllocatorTp = FixedAllocator<T>>
    class PagedAllocator : public IAllocator<T> {
        static_assert(std::is_base_of_v<IAllocator<T>, PageAllocatorTp>, "PageAllocatorTp template argument should be inherited from IAllocator");
        public:
            /*
            * @param args...
            *     Argument list matches the argument list for the constructor of PageAllocatorTp.
            *     By default this is FixedAllocator<T>.
            */
            template <typename ...Args>
            PagedAllocator(Args&& ...args);

            virtual ~PagedAllocator();

            virtual size_t totalCapacityBytes();

            virtual size_t maxAllocationSize();

            virtual bool isDynamicallySized();

            virtual size_t getPtrSize(void* ptr);

        protected:
            struct page_node {
                PageAllocatorTp* allocator;
                page_node* next;
            };

            virtual void* allocInternal(size_t count);

            virtual size_t freeInternal(void* ptr);

            virtual size_t resetInternal();

            // Hash map of pointer -> PageAllocatorTp*
            robin_hood::unordered_map<void*, PageAllocatorTp*> m_allocMap;
            std::function<page_node*()> m_newPageFunc;
            page_node* m_pages;
            u16 m_pageCount;
    };

    class GeneralAllocator : public IAllocator<void> {
        public:
            GeneralAllocator();
            virtual ~GeneralAllocator();

            virtual size_t totalCapacityBytes();

            virtual size_t maxAllocationSize();

            virtual bool isDynamicallySized();

            virtual size_t getPtrSize(void* ptr);

        protected:
            virtual void* allocInternal(size_t count);

            virtual size_t freeInternal(void* ptr);

            virtual size_t resetInternal();
    };

    template <typename T, typename AllocatorTp = FixedAllocator<T>>
    class PerThreadSingletonAllocator : public IAllocator<T>, public PerThreadSingleton<AllocatorTp> {
        static_assert(std::is_base_of_v<IAllocator<T>, AllocatorTp>, "AllocatorTp template argument should be inherited from IAllocator");
        public:
            static T* alloc(size_t sz);

            static T* realloc(T* ptr, size_t sz);

            static size_t free(T* ptr);

            static void copy(T* to, const T* from, size_t count);

            virtual size_t totalCapacityBytes();

            virtual size_t maxAllocationSize();

            virtual bool isDynamicallySized();

            virtual size_t getPtrSize(void* ptr);

        protected:
            virtual void* allocInternal(size_t count);

            virtual size_t freeInternal(void* ptr);

            virtual size_t resetInternal();

        private:
            PerThreadSingletonAllocator();

            virtual ~PerThreadSingletonAllocator();
    };


    typedef PerThreadSingletonAllocator<void, GeneralAllocator> Mem;
};