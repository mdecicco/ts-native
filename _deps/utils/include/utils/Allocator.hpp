#pragma once
#include <utils/Allocator.h>
#include <utils/Thread.h>

namespace utils {
    //
    // IAllocator
    //

    template <typename T, size_t ElemSz>
    size_t IAllocator<T, ElemSz>::ElementSize = ElemSz;

    template <typename T, size_t ElemSz>
    IAllocator<T, ElemSz>::IAllocator() : m_memUsed(0) { }

    template <typename T, size_t ElemSz>
    IAllocator<T, ElemSz>::~IAllocator() { }

    template <typename T, size_t ElemSz>
    T* IAllocator<T, ElemSz>::alloc(size_t count) {
        size_t bytes = ElemSz * count;
        if (!isDynamicallySized() && bytes > (totalCapacityBytes() - m_memUsed)) {
            // Engine::Get()->log(LogCode::insufficient_allocator_memory, bytes, totalCapacityBytes() - m_memUsed);
            return nullptr;
        }

        if (bytes > maxAllocationSize()) {
            // Engine::Get()->log(LogCode::allocation_too_large, bytes, maxAllocationSize());
            return nullptr;
        }

        T* out = (T*)allocInternal(count);
        if (out) m_memUsed += bytes;
        // else Engine::Get()->log(LogCode::allocation_failed, bytes, ElemSz, count);
        return out;
    }

    template <typename T, size_t ElemSz>
    T* IAllocator<T, ElemSz>::realloc(T* ptr, size_t count) {
        size_t oldSz = getPtrSize(ptr);
        if (oldSz == 0) return nullptr;

        T* newMem = alloc(count);
        if (!newMem) return nullptr;

        copy(newMem, ptr, oldSz / ElemSz);

        return newMem;
    }

    template <typename T, size_t ElemSz>
    size_t IAllocator<T, ElemSz>::free(T* ptr) {
        size_t ptrSize = freeInternal(ptr);
        if (ptrSize > 0) {
            m_memUsed -= ptrSize;
            return ptrSize;
        }
        // Engine::Get()->log(LogCode::deallocation_failed, (void*)ptr);
        return 0;
    }

    template <typename T, size_t ElemSz>
    size_t IAllocator<T, ElemSz>::reset() {
        size_t totalFreed = resetInternal();

        if (totalFreed > m_memUsed) {
            // todo: exception
            m_memUsed = 0;
        } else m_memUsed -= totalFreed;

        return totalFreed;
    }

    template <typename T, size_t ElemSz>
    void IAllocator<T, ElemSz>::copy(T* to, const T* from, size_t count) {
        if (!from || !to || !count) return;

        if constexpr (std::is_void_v<T> || std::is_trivially_copyable_v<T>) {
            std::memcpy(to, from, count * ElemSz);
        } else {
            for (size_t i = 0;i < count;i++) new (to + i) T(from[i]);
        }
    }

    template <typename T, size_t ElemSz>
    size_t IAllocator<T, ElemSz>::elemSize() const {
        return ElemSz;
    }

    template <typename T, size_t ElemSz>
    size_t IAllocator<T, ElemSz>::memUsed() const {
        return m_memUsed;
    }



    //
    // FixedAllocator
    //

    template <typename T>
    FixedAllocator<T>::FixedAllocator(size_t elementCapacity, size_t maxLiveAllocations, bool zeroMem)
        : IAllocator<T>(),
        m_byteCapacity(elementCapacity * IAllocator<T>::ElementSize),
        m_elementCapacity(elementCapacity),
        m_doZeroMem(zeroMem)
    {
        if (maxLiveAllocations == 0) maxLiveAllocations = elementCapacity;

        size_t totalSz = (IAllocator<T>::ElementSize * elementCapacity) + (sizeof(elem_node) * maxLiveAllocations);
        m_memBase = new u8[totalSz];

        m_nodeBase = m_memBase + m_byteCapacity;

        elem_node* n = (elem_node*)m_nodeBase;
        n->last = nullptr;

        for (size_t i = 0;i < maxLiveAllocations;i++) {
            n->ptr = nullptr;
            n->count = 0;
            n->used = false;
            n->next = ((i + 1) < maxLiveAllocations) ? n + 1 : nullptr;
            if (n->next) n->next->last = n;
            n++;
        }

        m_freeList = (elem_node*)m_nodeBase;

        m_memList = getEmptyNode();
        m_memList->ptr = m_memBase;
        m_memList->count = elementCapacity;
    }

    template <typename T>
    FixedAllocator<T>::~FixedAllocator<T>() {
        delete [] m_memBase;
        m_memBase = m_nodeBase = nullptr;
        m_freeList = m_memList = nullptr;
        m_byteCapacity = m_elementCapacity = 0;
    }

    template <typename T>
    size_t FixedAllocator<T>::totalCapacityBytes() {
        return m_byteCapacity;
    }

    template <typename T>
    size_t FixedAllocator<T>::maxAllocationSize() {
        return m_byteCapacity;
    }

    template <typename T>
    bool FixedAllocator<T>::isDynamicallySized() {
        return false;
    }

    template <typename T>
    void* FixedAllocator<T>::allocInternal(size_t count) {
        if (count == 0) return nullptr;

        elem_node* last = nullptr;
        elem_node* n = m_memList;
        while (n) {
            if (n->used) {
                last = n;
                n = n->next;
                continue;
            }

            if (n->count == count) {
                // node will work as-is

                break;
            } else if (n->count > count) {
                // need to split the node

                elem_node* remainder = getEmptyNode();
                if (!remainder) {
                    // Engine::Get()->log(LogCode::max_allocations_exceeded);
                    return nullptr;
                }

                remainder->count = n->count - count;
                remainder->ptr = (u8*)(((T*)n->ptr) + count);
                remainder->next = n->next;
                remainder->last = n;

                if (n->next) n->next->last = remainder;
                n->next = remainder;
                n->count = count;
                break;
            }

            last = n;
            n = n->next;
        }

        if (n) {
            n->used = true;
            u8* ptr = n->ptr;
            m_liveMap[ptr] = n;

            if (m_doZeroMem) {
                size_t sz = count * IAllocator<T>::ElementSize;
                // Iterate over memory 8 bytes at a time and zero the whole 8 bytes at once
                size_t bytesZeroed = 0;
                for (;(bytesZeroed + sizeof(u64)) < sz;bytesZeroed += sizeof(u64)) {
                    *((u64*)(ptr + bytesZeroed)) = 0;
                }

                // If totalSz is not evenly divisible by 8, iterate over the remaining few
                // bytes and just zero them one at a time
                if (bytesZeroed < sz) {
                    for (size_t i = bytesZeroed;i < sz;i++) ptr[i] = 0;
                }
            }

            return ptr;
        }

        return nullptr;
    }

    template <typename T>
    size_t FixedAllocator<T>::freeInternal(void* ptr) {
        auto it = m_liveMap.find(ptr);
        if (it == m_liveMap.end()) return 0;

        elem_node* n = it->second;
        size_t ptrSize = n->count * IAllocator<T>::ElementSize;
        m_liveMap.erase(it);
        n->used = false;

        // if the block before n is unused, merge it
        if (n->last && !n->last->used) {
            elem_node* prev = n->last;
            prev->count += n->count;

            prev->next = n->next;
            if (n->next) n->next->last = prev;
            releaseNode(n);

            n = prev;
        }

        // if the block after n is unused, merge it
        if (n->next && !n->next->used) {
            elem_node* next = n->next;
            n->count += next->count;

            n->next = next->next;
            if (next->next) next->next->last = n;
            releaseNode(next);
        }

        return ptrSize;
    }

    template <typename T>
    size_t FixedAllocator<T>::resetInternal() {
        m_liveMap.clear();
        m_freeList = (elem_node*)m_nodeBase;
        m_memList = getEmptyNode();
        m_memList->ptr = m_memBase;
        m_memList->count = m_elementCapacity;
        return this->memUsed();
    }

    template <typename T>
    size_t FixedAllocator<T>::getPtrSize(void* ptr) {
        auto it = m_liveMap.find(ptr);
        if (it == m_liveMap.end()) return 0;

        elem_node* n = it->second;
        return n->count * IAllocator<T>::ElementSize;
    }

    template <typename T>
    typename FixedAllocator<T>::elem_node* FixedAllocator<T>::getEmptyNode() {
        if (!m_freeList) return nullptr;

        elem_node* out = m_freeList;
        m_freeList = m_freeList->next;
        if (m_freeList) m_freeList->last = nullptr;

        out->next = out->last = nullptr;
        out->used = false;
        out->ptr = nullptr;
        out->count = 0;

        return out;
    }

    template <typename T>
    void FixedAllocator<T>::releaseNode(elem_node* node) {
        node->next = nullptr;
        node->last = nullptr;
        node->count = 0;
        node->used = false;
        node->ptr = nullptr;

        if (!m_freeList) m_freeList = node;
        else {
            node->next = m_freeList;
            m_freeList->last = node;
            m_freeList = node;
        }
    }



    //
    // PagedAllocator
    //

    template <typename T, typename PageAllocatorTp>
    template <typename ...Args>
    PagedAllocator<T, PageAllocatorTp>::PagedAllocator(Args&& ...args)
        : IAllocator<T>(),
        m_pages(nullptr),
        m_pageCount(1)
    {
        m_newPageFunc = [this, ...args = std::forward<Args>(args)]() {
            return new page_node({ new PageAllocatorTp(args...), m_pages });
        };
        m_pages = m_newPageFunc();
    }

    template <typename T, typename PageAllocatorTp>
    PagedAllocator<T, PageAllocatorTp>::~PagedAllocator<T, PageAllocatorTp>() {
        page_node* n = m_pages;
        while (n) {
            delete n->allocator;
            page_node* next = n->next;
            delete n;
            n = next;
        }
    }

    template <typename T, typename PageAllocatorTp>
    size_t PagedAllocator<T, PageAllocatorTp>::totalCapacityBytes() {
        return m_pages ? m_pages->allocator->totalCapacityBytes() * m_pageCount : 0;
    }

    template <typename T, typename PageAllocatorTp>
    size_t PagedAllocator<T, PageAllocatorTp>::maxAllocationSize() {
        return m_pages ? m_pages->allocator->totalCapacityBytes() : 0;
    }

    template <typename T, typename PageAllocatorTp>
    bool PagedAllocator<T, PageAllocatorTp>::isDynamicallySized() {
        return true;
    }

    template <typename T, typename PageAllocatorTp>
    void* PagedAllocator<T, PageAllocatorTp>::allocInternal(size_t count) {
        if (count == 0) return nullptr;

        page_node* n = m_pages;
        while (n) {
            // bool wasSuppressed = Engine::Get()->isLoggingSuppressed();
            // Engine::Get()->setLogsSuppressed(true);

            void* ret = n->allocator->alloc(count);

            // Engine::Get()->setLogsSuppressed(wasSuppressed);

            if (ret) {
                m_allocMap[ret] = n->allocator;
                return ret;
            }
            n = n->next;
        }

        // Try allocating from a new page
        m_pages = m_newPageFunc();
        m_pageCount++;

        void* ret = m_pages->allocator->alloc(count);
        if (ret) m_allocMap[ret] = m_pages->allocator;
        return ret;
    }

    template <typename T, typename PageAllocatorTp>
    size_t PagedAllocator<T, PageAllocatorTp>::freeInternal(void* ptr) {
        auto it = m_allocMap.find(ptr);
        if (it == m_allocMap.end()) return 0;

        PageAllocatorTp* alloc = it->second;
        m_allocMap.erase(it);

        return alloc->free((T*)ptr);
    }

    template <typename T, typename PageAllocatorTp>
    size_t PagedAllocator<T, PageAllocatorTp>::resetInternal() {
        size_t totalFreed = 0;

        u16 i = 0;
        page_node* n = m_pages;
        while (n) {
            totalFreed += n->allocator->reset();

            if (i > 0) {
                delete n->allocator;
                delete n;
            }

            n = n->next;
            i++;
        }

        m_pages->next = nullptr;

        return totalFreed;
    }

    template <typename T, typename PageAllocatorTp>
    size_t PagedAllocator<T, PageAllocatorTp>::getPtrSize(void* ptr) {
        auto it = m_allocMap.find(ptr);
        if (it == m_allocMap.end()) return 0;

        return it->second->getPtrSize((T*)ptr);
    }


    //
    // PerThreadSingletonAllocator
    //

    template <typename T, typename AllocatorTp>
    PerThreadSingletonAllocator<T, AllocatorTp>::PerThreadSingletonAllocator() : IAllocator<T>() {
    }

    template <typename T, typename AllocatorTp>
    PerThreadSingletonAllocator<T, AllocatorTp>::~PerThreadSingletonAllocator<T, AllocatorTp>() {
    }

    template <typename T, typename AllocatorTp>
    T* PerThreadSingletonAllocator<T, AllocatorTp>::alloc(size_t sz) {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->alloc(sz);
    }

    template <typename T, typename AllocatorTp>
    T* PerThreadSingletonAllocator<T, AllocatorTp>::realloc(T* ptr, size_t sz) {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->realloc(ptr, sz);
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::free(T* ptr) {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->free(ptr);
    }

    template <typename T, typename AllocatorTp>
    void PerThreadSingletonAllocator<T, AllocatorTp>::copy(T* to, const T* from, size_t count) {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->copy(to, from, count);
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::totalCapacityBytes() {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->totalCapacityBytes();
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::maxAllocationSize() {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->maxAllocationSize();
    }

    template <typename T, typename AllocatorTp>
    bool PerThreadSingletonAllocator<T, AllocatorTp>::isDynamicallySized() {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->isDynamicallySized();
    }

    template <typename T, typename AllocatorTp>
    void* PerThreadSingletonAllocator<T, AllocatorTp>::allocInternal(size_t count) {
        // bool wasSuppressed = Engine::Get()->isLoggingSuppressed();
        // Engine::Get()->setLogsSuppressed(true);

        void* ret = PerThreadSingletonAllocator<T, AllocatorTp>::Get()->alloc(count);

        // Engine::Get()->setLogsSuppressed(wasSuppressed);
        return ret;
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::freeInternal(void* ptr) {
        // bool wasSuppressed = Engine::Get()->isLoggingSuppressed();
        // Engine::Get()->setLogsSuppressed(true);

        size_t ret = PerThreadSingletonAllocator<T, AllocatorTp>::Get()->free((T*)ptr);

        // Engine::Get()->setLogsSuppressed(wasSuppressed);
        return ret;
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::getPtrSize(void* ptr) {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->getPtrSize(ptr);
    }

    template <typename T, typename AllocatorTp>
    size_t PerThreadSingletonAllocator<T, AllocatorTp>::resetInternal() {
        return PerThreadSingletonAllocator<T, AllocatorTp>::Get()->reset();
    }
};