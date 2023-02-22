#pragma once
#include <utils/Array.h>
#include <utils/Singleton.hpp>
#include <utils/Allocator.hpp>

namespace utils {
    //
    // ArrayIterator
    //

    template <typename T>
    ArrayIterator<T>::ArrayIterator(T* ele) : m_ptr(ele) {
    }

    template <typename T>
    typename ArrayIterator<T>::reference ArrayIterator<T>::operator* () const {
        return *m_ptr;
    }

    template <typename T>
    typename ArrayIterator<T>::pointer ArrayIterator<T>::operator->() const {
        return m_ptr;
    }

    template <typename T>
    ArrayIterator<T>& ArrayIterator<T>::operator++() {
        m_ptr++;
        return *this;
    }

    template <typename T>
    ArrayIterator<T>& ArrayIterator<T>::operator--() {
        m_ptr--;
        return *this;
    }

    template <typename T>
    typename ArrayIterator<T>::difference_type ArrayIterator<T>::operator-(const ArrayIterator<T>& it) const {
        return m_ptr - it.m_ptr;
    }

    template <typename T>
    ArrayIterator<T> ArrayIterator<T>::operator+(const difference_type& diff) const {
        return ArrayIterator<T>(m_ptr + diff);
    }

    template <typename T>
    ArrayIterator<T> ArrayIterator<T>::operator-(const difference_type& diff) const {
        return ArrayIterator<T>(m_ptr - diff);
    }

    template <typename T>
    typename ArrayIterator<T>::reference ArrayIterator<T>::operator[] (const difference_type& offset) const {
        return m_ptr[offset];
    }

    template <typename T>
    bool ArrayIterator<T>::operator==(const ArrayIterator<T>& it) const {
        return m_ptr == it.m_ptr;
    }

    template <typename T>
    bool ArrayIterator<T>::operator!=(const ArrayIterator<T>& it) const {
        return m_ptr != it.m_ptr;
    }

    template <typename T>
    bool ArrayIterator<T>::operator<(const ArrayIterator<T>& it) const {
        return m_ptr < it.m_ptr;
    }

    template <typename T>
    bool ArrayIterator<T>::operator>(const ArrayIterator<T>& it) const {
        return m_ptr > it.m_ptr;
    }

    template <typename T>
    bool ArrayIterator<T>::operator>=(const ArrayIterator<T>& it) const {
        return m_ptr >= it.m_ptr;
    }

    template <typename T>
    bool ArrayIterator<T>::operator<=(const ArrayIterator<T>& it) const {
        return m_ptr <= it.m_ptr;
    }

    template<typename T>
    ArrayIterator<T>::operator ArrayIterator<const T>() const {
        return ArrayIterator<const T>(m_ptr);
    }



    //
    // ArrayView
    //

    template <typename T>
    ArrayView<T>::ArrayView(Array<T>* target, u32 from, u32 size) : m_target(target), m_offset(from), m_size(size) {
    }

    template <typename T>
    ArrayView<T>::~ArrayView<T>() {
    }

    template <typename T>
    typename ArrayView<T>::iterator ArrayView<T>::begin() {
        if (m_offset > m_target->m_size) return m_target->m_data + m_target->m_size;
        return m_target->m_data + m_offset;
    }

    template <typename T>
    typename ArrayView<T>::const_iterator ArrayView<T>::begin() const {
        if (m_offset > m_target->m_size) return m_target->m_data + m_target->m_size;
        return m_target->m_data + m_offset;
    }

    template <typename T>
    typename ArrayView<T>::iterator ArrayView<T>::end() {
        size_t end = m_offset + m_size;
        if (end > m_target->m_size) end = m_target->m_size;
        return m_target->m_data + end;
    }

    template <typename T>
    T& ArrayView<T>::last() {
        return *(end() - 1);
    }

    template <typename T>
    const T& ArrayView<T>::last() const {
        return *(end() - 1);
    }

    template <typename T>
    typename ArrayView<T>::const_iterator ArrayView<T>::end() const {
        size_t end = m_offset + m_size;
        if (end > m_target->m_size) end = m_target->m_size;
        return m_target->m_data + end;
    }

    template <typename T>
    u32 ArrayView<T>::size() const {
        if (m_offset >= m_target->m_size) return 0;
        
        if ((m_offset + m_size) >= m_target->m_size) return m_target->m_size - m_offset;

        return m_size;
    }

    template <typename T>
    u32 ArrayView<T>::viewOffset() const {
        return m_offset;
    }

    template <typename T>
    u32 ArrayView<T>::viewSize() const {
        return m_size;
    }

    template <typename T>
    Array<T>* ArrayView<T>::target() const {
        return m_target;
    }

    template <typename T>
    T& ArrayView<T>::operator[](u32 idx) {
        return m_target->operator[](m_offset + idx);
    }

    template <typename T>
    const T& ArrayView<T>::operator[](u32 idx) const {
        return m_target->operator[](m_offset + idx);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T>> ArrayView<T>::each(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(m_target->m_data[i]);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(m_target->m_data[i]);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(&m_target->m_data[i]);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T, u32>> ArrayView<T>::each(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(m_target->m_data[i], i - m_offset);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&, u32>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(m_target->m_data[i], i - m_offset);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*, u32>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            cb(&m_target->m_data[i], i - m_offset);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>> ArrayView<T>::each(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(&m_target->m_data[i])) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>> ArrayView<T>::each(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&, u32>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*, u32>> ArrayView<T>::each(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(&m_target->m_data[i], i - m_offset)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> ArrayView<T>::some(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, bool> ArrayView<T>::some(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, Array<T>> ArrayView<T>::filter(F&& cb) const {
        Array<T> out;

        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) out.push(m_target->m_data[i]);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, Array<T>> ArrayView<T>::filter(F&& cb) const {
        Array<T> out;

        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) out.push(m_target->m_data[i]);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const typename ArrayView<T>::PointerTp> ArrayView<T>::find(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) {
                if constexpr (std::is_pointer_v<T>) return m_target->m_data[i];
                else return &m_target->m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, typename ArrayView<T>::PointerTp> ArrayView<T>::find(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) {
                if constexpr (std::is_pointer_v<T>) return m_target->m_data[i];
                else return &m_target->m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, const typename ArrayView<T>::PointerTp> ArrayView<T>::find(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) {
                if constexpr (std::is_pointer_v<T>) return m_target->m_data[i];
                else return &m_target->m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, typename ArrayView<T>::PointerTp> ArrayView<T>::find(F&& cb) {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) {
                if constexpr (std::is_pointer_v<T>) return m_target->m_data[i];
                else return &m_target->m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, i64> ArrayView<T>::findIndex(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i])) return i;
        }

        return -1;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, i64> ArrayView<T>::findIndex(F&& cb) const {
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) {
            if (cb(m_target->m_data[i], i - m_offset)) return i;
        }

        return -1;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T>, Array<M>> ArrayView<T>::map(F&& cb) const {
        Array<M> out(m_size);
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) out.push(cb(m_target->m_data[i]));
        return out;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T, u32>, Array<M>> ArrayView<T>::map(F&& cb) const {
        Array<M> out(m_size);
        for (u32 i = m_offset;i < (m_offset + m_size) && i < m_target->m_size;i++) out.push(cb(m_target->m_data[i], i - m_offset));
        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> ArrayView<T>::sort(F&& cb) {
        std::sort(begin(), end(), cb);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, Array<T>> ArrayView<T>::sorted(F&& cb) const {
        Array<T> out = *this;
        std::sort(out.begin(), out.end(), cb);
        return out;
    }



    //
    // Array
    //

    template <typename T>
    Array<T>::Array() : m_size(0), m_capacity(0), m_data(nullptr) {
    }

    template <typename T>
    Array<T>::Array(const Array<T>& rhs) : m_size(0), m_capacity(0), m_data(nullptr) {
        reserve(rhs.m_size);
        m_size = rhs.m_size;
        for (u32 i = 0;i < rhs.m_size;i++) new (m_data + i) T(rhs.m_data[i]);
    }

    template <typename T>
    Array<T>::Array(u32 reserveCount) : m_size(0), m_capacity(0), m_data(nullptr) {
        reserve(reserveCount);
    }

    template <typename T>
    Array<T>::Array(std::initializer_list<T> list) : m_size(0), m_capacity(0), m_data(nullptr) {
        reserve((u32)list.size());
        for (T ele : list) {
            new (&m_data[m_size++]) T(ele);
        }
    }

    template <typename T>
    Array<T>::Array(const ArrayView<T>& rhs) : m_size(0), m_capacity(0), m_data(nullptr) {
        reserve(rhs.size());
        for (T& ele : rhs) {
            new (&m_data[m_size++]) T(ele);
        }
    }

    template <typename T>
    Array<T>::~Array<T>() {
        if (m_data) Mem::free(m_data);
        m_data = nullptr;
    }


    template <typename T>
    typename Array<T>::iterator Array<T>::begin() {
        //return ArrayIterator<T>(this, 0);
        return Array<T>::iterator(m_data);
    }

    template <typename T>
    typename Array<T>::const_iterator Array<T>::begin() const {
        //return ConstArrayIterator<T>(this, 0);
        return Array<T>::const_iterator(m_data);
    }

    template <typename T>
    typename Array<T>::iterator Array<T>::end() {
        //return ArrayIterator<T>(this, m_size);
        return Array<T>::iterator(m_data + m_size);
    }

    template <typename T>
    typename Array<T>::const_iterator Array<T>::end() const {
        //return ConstArrayIterator<T>(this, m_size);
        return Array<T>::const_iterator(m_data + m_size);
    }

    template <typename T>
    T* Array<T>::data() {
        return m_data;
    }

    template <typename T>
    const T* Array<T>::data() const {
        return m_data;
    }

    template <typename T>
    T& Array<T>::last() {
        return *(end() - 1);
    }

    template <typename T>
    const T& Array<T>::last() const {
        return *(end() - 1);
    }

    template <typename T>
    template <typename ...Args>
    void Array<T>::emplace(Args... args) {
        if (m_size + 1 >= m_capacity) expand();

        new (&m_data[m_size++]) T(args...);
    }

    template <typename T>
    template <typename ...Args>
    void Array<T>::emplaceAt(u32 at, Args... args) {
        if (m_size + 1 >= m_capacity) expand();

        for (u32 i = m_size;i > at;i--) {
            m_data[i] = m_data[i - 1];
        }

        new (m_data + at) T(args...);
        m_size++;
    }


    template <typename T>
    void Array<T>::push(const T& v) {
        if (m_size + 1 >= m_capacity) expand();
        new (&m_data[m_size++]) T(v);
    }

    template <typename T>
    void Array<T>::insert(u32 at, const T& v) {
        if (m_size + 1 >= m_capacity) expand();

        for (u32 i = m_size;i > at;i--) {
            if (i >= m_size) new (&m_data[i]) T(m_data[i - 1]);
            else m_data[i] = m_data[i - 1];
        }

        m_data[at] = v;
        m_size++;
    }

    template <typename T>
    void Array<T>::insert(u32 at, const Array<T>& v) {
        if (v.m_size == 0) return;

        if (m_size + v.m_size >= m_capacity) expand(v.m_size);

        for (u32 i = m_size + v.m_size - 1;i >= (at + v.m_size);i--) {
            if (i >= m_size) new (&m_data[i]) T(m_data[i - v.m_size]);
            else m_data[i] = m_data[i - v.m_size];
        }

        for (u32 i = 0;i < v.m_size;i++) m_data[at + i] = v.m_data[i];

        m_size += v.m_size;
    }

    template <typename T>
    void Array<T>::insert(u32 at, std::initializer_list<T> list) {
        u32 sz = (u32)list.size();
        if (sz == 0) return;

        if (m_size + sz >= m_capacity) expand(sz);

        for (u32 i = m_size + sz - 1;i >= (at + sz);i--) {
            if (i >= m_size) new (&m_data[i]) T(m_data[i - sz]);
            else m_data[i] = m_data[i - sz];
        }

        u32 i = 0;
        for (auto ele : list) {
            m_data[at + i] = ele;
            i++;
        }

        m_size += sz;
    }

    template <typename T>
    void Array<T>::swap(u32 a, u32 b) {
        T tmp = m_data[a];
        m_data[a] = m_data[b];
        m_data[b] = tmp;
    }

    template <typename T>
    T Array<T>::pop(bool fromFront) {
        if (m_size == 0) {
            // todo exception
        }

        if (fromFront) {
            T result = m_data[0];
            for (u32 i = 0;i < m_size - 1;i++) m_data[i] = m_data[i + 1];
            m_size--;
            return result;
        } else m_size--;

        return m_data[m_size];
    }

    template <typename T>
    void Array<T>::remove(u32 from, u32 count) {
        if (m_size == 0 || count == 0) return;
        if (from + count >= m_size) {
            m_size = from;
            return;
        }

        for (u32 i = 0;(from + i + count) < m_size;i++) {
            m_data[from + i] = m_data[from + i + count];
        }

        m_size -= count;
    }

    template <typename T>
    Array<T> Array<T>::extract(u32 from, u32 count, bool doRemove) {
        Array<T> out;

        if (m_size == 0 || count == 0) return out;
        if (from + count >= m_size) count = m_size - from;
        out.reserve(count);

        for (u32 i = 0;i < count;i++) {
            out.push(m_data[from + i]);
            if (doRemove && (from + i + count) < m_size) m_data[from + i] = m_data[from + i + count];
        }

        if (doRemove) m_size -= count;
        return out;
    }

    template <typename T>
    ArrayView<T> Array<T>::view(u32 from, u32 count) {
        return ArrayView<T>(this, from, count);
    }

    template <typename T>
    void Array<T>::clear(bool doFreeMem) {
        m_size = 0;
        if (doFreeMem) {
            if (m_data) Mem::free(m_data);
            m_capacity = 0;
            m_data = nullptr;
        }
    }

    template <typename T>
    void Array<T>::reserve(u32 count) {
        if (m_size + count < m_capacity) return;
        expand((m_size + count) - m_capacity);
    }

    template <typename T>
    u32 Array<T>::size() const {
        return m_size;
    }

    template <typename T>
    u32 Array<T>::capacity() const {
        return m_capacity;
    }

    template <typename T>
    Array<T> Array<T>::concat(const Array<T>& arr) const {
        if ((m_size + arr.m_size) == 0) return Array<T>();

        Array<T> out(m_size + arr.m_size);
        for (u32 i = 0;i < m_size;i++) out.m_data[i] = m_data[i];
        for (u32 i = 0;i < arr.m_size;i++) out.m_data[i + m_size] = arr.m_data[i];
        out.m_size = m_size + arr.m_size;

        return out;
    }

    template <typename T>
    Array<T> Array<T>::concat(std::initializer_list<T> list) const {
        u32 sz = (u32)list.size();
        if ((m_size + sz) == 0) return Array<T>();

        Array<T> out(m_size + sz);
        for (u32 i = 0;i < m_size;i++) out.m_data[i] = m_data[i];

        u32 i = 0;
        for (auto ele : list) {
            out.m_data[m_size + i] = ele;
            i++;
        }

        out.m_size = m_size + sz;
        
        return out;
    }

    template <typename T>
    void Array<T>::append(const Array<T>& arr) {
        if (arr.m_size == 0) return;
        reserve(arr.m_size);
        for (u32 i = 0;i < arr.m_size;i++) m_data[m_size++] = arr[i];
    }

    template <typename T>
    void Array<T>::append(std::initializer_list<T> list) {
        u32 sz = (u32)list.size();
        if (sz == 0) return;
        reserve(sz);

        for (auto ele : list) m_data[m_size++] = ele;
    }

    template <typename T>
    T& Array<T>::operator[](u32 idx) {
        // todo exceptions
        return m_data[idx];
    }

    template <typename T>
    const T& Array<T>::operator[](u32 idx) const {
        // todo exceptions
        return m_data[idx];
    }

    template <typename T>
    Array<T>& Array<T>::operator=(const Array<T>& rhs) {
        clear(m_capacity < rhs.m_size);
        if (m_capacity < rhs.m_size) reserve(rhs.m_size);
        for (u32 i = 0;i < rhs.m_size;i++) new (&m_data[m_size++]) T(rhs.m_data[i]);
        return *this;
    }

    template <typename T>
    Array<T>& Array<T>::operator=(std::initializer_list<T> list) {
        u32 sz = (u32)list.size();
        clear(m_capacity < sz);
        if (m_capacity < sz) reserve(sz);

        for (T ele : list) {
            new (&m_data[m_size++]) T(ele);
        }
        return *this;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T>> Array<T>::each(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) cb(m_data[i]);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) cb(m_data[i]);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) cb(m_data + i);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T, u32>> Array<T>::each(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) cb(m_data[i], i);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&, u32>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) cb(m_data[i], i);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*, u32>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) cb(m_data + i, i);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>> Array<T>::each(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data + i)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>> Array<T>::each(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&, u32>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*, u32>> Array<T>::each(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data + i, i)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> Array<T>::some(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, bool> Array<T>::some(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, Array<T>> Array<T>::filter(F&& cb) const {
        Array<T> out;

        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) out.push(m_data[i]);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, Array<T>> Array<T>::filter(F&& cb) const {
        Array<T> out;

        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) out.push(m_data[i]);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const typename Array<T>::PointerTp> Array<T>::find(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) {
                if constexpr (std::is_pointer_v<T>) return m_data[i];
                else return &m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, typename Array<T>::PointerTp> Array<T>::find(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) {
                if constexpr (std::is_pointer_v<T>) return m_data[i];
                else return &m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, const typename Array<T>::PointerTp> Array<T>::find(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) {
                if constexpr (std::is_pointer_v<T>) return m_data[i];
                else return &m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, typename Array<T>::PointerTp> Array<T>::find(F&& cb) {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) {
                if constexpr (std::is_pointer_v<T>) return m_data[i];
                else return &m_data[i];
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, i64> Array<T>::findIndex(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i])) return i;
        }

        return -1;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, i64> Array<T>::findIndex(F&& cb) const {
        for (u32 i = 0;i < m_size;i++) {
            if (cb(m_data[i], i)) return i;
        }

        return -1;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T>, Array<M>> Array<T>::map(F&& cb) const {
        Array<M> out(m_size);
        for (u32 i = 0;i < m_size;i++) out.push(cb(m_data[i]));
        return out;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T, u32>, Array<M>> Array<T>::map(F&& cb) const {
        Array<M> out(m_size);
        for (u32 i = 0;i < m_size;i++) out.push(cb(m_data[i], i));
        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> Array<T>::sort(F&& cb) {
        std::sort(begin(), end(), cb);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, Array<T>> Array<T>::sorted(F&& cb) const {
        Array<T> out = *this;
        std::sort(out.begin(), out.end(), cb);
        return out;
    }

    template <typename T>
    void Array<T>::expand(u32 minDelta) {
        if (!m_data) {
            u32 prevCap = m_capacity;
            if (m_capacity == 0) m_capacity = 16;
            while ((m_capacity - prevCap) < minDelta) m_capacity *= 2;
            m_data = (T*)Mem::alloc(sizeof(T) * m_capacity);
            return;
        }

        u32 prevCap = m_capacity;
        while ((m_capacity - prevCap) < minDelta) m_capacity *= 2;
        m_data = (T*)Mem::realloc(m_data, sizeof(T) * m_capacity);
    }
};