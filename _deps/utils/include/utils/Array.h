#pragma once
#include <utils/Types.h>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <xtr1common>

namespace utils {
    template <typename T>
    class Array;

    template <typename T>
    class ArrayIterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = T;
            using reference         = T&;
            using pointer           = T*;
            using difference_type   = unsigned long long;

            ArrayIterator(T* ele);

            reference operator* () const;
            pointer operator->() const;

            ArrayIterator<T>& operator++();
            ArrayIterator<T>& operator--();

            difference_type operator-(const ArrayIterator<T>& it) const;

            ArrayIterator<T> operator+(const difference_type& diff) const;
            ArrayIterator<T> operator-(const difference_type& diff) const;

            reference operator[] (const difference_type& offset) const;

            bool operator==(const ArrayIterator<T>& it) const;
            bool operator!=(const ArrayIterator<T>& it) const;
            bool operator<(const ArrayIterator<T>& it) const;
            bool operator>(const ArrayIterator<T>& it) const;
            bool operator>=(const ArrayIterator<T>& it) const;
            bool operator<=(const ArrayIterator<T>& it) const;

            operator ArrayIterator<const T>() const;

        protected:
            T* m_ptr;
    };

    template <typename T>
    class ArrayView {
        public:
            using iterator = ArrayIterator<T>;
            using const_iterator = ArrayIterator<const T>;
            using PointerTp = std::remove_pointer_t<std::remove_cvref_t<T>>*;

            ArrayView(Array<T>* target, u32 from, u32 size);
            ~ArrayView();

            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            T& last();
            const T& last() const;

            u32 size() const;
            u32 viewOffset() const;
            u32 viewSize() const;
            Array<T>* target() const;

            T& operator[](u32 idx);
            const T& operator[](u32 idx) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*>> each(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T, u32>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&, u32>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*, u32>> each(F&& cb);

            // element -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> each(F&& cb);

            // element, index -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&, u32>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*, u32>> each(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> some(F&& cb) const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, bool> some(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, Array<T>> filter(F&& cb)  const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, Array<T>> filter(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, PointerTp> find(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, PointerTp> find(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, i64> findIndex(F&& cb) const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, i64> findIndex(F&& cb) const;

            // element
            template <typename F, typename M = std::invoke_result_t<F, T>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T>, Array<M>> map(F&& cb) const;

            // element, index
            template <typename F, typename M = std::invoke_result_t<F, T, u32>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T, u32>, Array<M>> map(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> sort(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, Array<T>> sorted(F&& cb) const;

        protected:
            Array<T>* m_target;
            u32 m_offset;
            u32 m_size;
    };

    template <typename T>
    class Array {
        public:
            using iterator = ArrayIterator<T>;
            using const_iterator = ArrayIterator<const T>;
            using PointerTp = std::remove_pointer_t<std::remove_cvref_t<T>>*;

            Array();
            Array(const Array<T>& rhs);
            Array(u32 reserveCount);
            Array(std::initializer_list<T> list);
            Array(const ArrayView<T>& rhs);
            ~Array();

            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            T* data();
            const T* data() const;
            T& last();
            const T& last() const;

            template <typename ...Args> void emplace(Args... args);
            template <typename ...Args> void emplaceAt(u32 at, Args... args);

            void push(const T& v);
            void insert(u32 at, const T& v);
            void insert(u32 at, const Array<T>& v);
            void insert(u32 at, std::initializer_list<T> list);
            void swap(u32 a, u32 b);
            
            T pop(bool fromFront = false);
            void remove(u32 from, u32 count = 1);
            Array<T> extract(u32 from, u32 count = UINT_MAX, bool doRemove = true);
            ArrayView<T> view(u32 from, u32 count = UINT_MAX);

            void clear(bool doFreeMem = false);
            void reserve(u32 count);
            u32 size() const;
            u32 capacity() const;

            Array<T> concat(const Array<T>& arr) const;
            Array<T> concat(std::initializer_list<T> list) const;
            void append(const Array<T>& arr);
            void append(std::initializer_list<T> list);

            T& operator[](u32 idx);
            const T& operator[](u32 idx) const;
            Array<T>& operator=(const Array<T>& rhs);
            Array<T>& operator=(std::initializer_list<T> list);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*>> each(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T, u32>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&, u32>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*, u32>> each(F&& cb);

            // element -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> each(F&& cb);

            // element, index -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&, u32>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*, u32>> each(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> some(F&& cb) const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, bool> some(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, Array<T>> filter(F&& cb)  const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, Array<T>> filter(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, PointerTp> find(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, PointerTp> find(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, i64> findIndex(F&& cb) const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, u32>, i64> findIndex(F&& cb) const;

            // element
            template <typename F, typename M = std::invoke_result_t<F, T>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T>, Array<M>> map(F&& cb) const;

            // element, index
            template <typename F, typename M = std::invoke_result_t<F, T, u32>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T, u32>, Array<M>> map(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> sort(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, Array<T>> sorted(F&& cb) const;

        protected:
            friend class ArrayView<T>;
            void expand(u32 minDelta = 1);

            u32 m_size;
            u32 m_capacity;
            T* m_data;
    };
};