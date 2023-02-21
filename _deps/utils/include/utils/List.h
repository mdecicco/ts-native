#pragma once
#include <utils/Types.h>
#include <utils/Allocator.h>

namespace utils {
    template <typename T>
    class BidirectionalList;

    template <typename T>
    class BidirectionalListIterator;

    template <typename T>
    struct BidirectionalListNode {
        public:
            T value;

        private:
            friend class BidirectionalList<T>;
            friend class BidirectionalListIterator<T>;
            BidirectionalListNode<T>* m_prev;
            BidirectionalListNode<T>* m_next;
    };

    template <typename T>
    class BidirectionalListIterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = T;
            using reference         = T&;
            using pointer           = T*;
            using difference_type   = unsigned long long;

            BidirectionalListIterator(BidirectionalListNode<T>* node);

            reference operator* () const;
            pointer operator->() const;

            BidirectionalListIterator<T>& operator++();
            BidirectionalListIterator<T>& operator--();

            difference_type operator-(const BidirectionalListIterator<T>& it) const;

            BidirectionalListIterator<T> operator+(const difference_type& diff) const;
            BidirectionalListIterator<T> operator-(const difference_type& diff) const;

            reference operator[] (const difference_type& offset) const;

            bool operator==(const BidirectionalListIterator<T>& it) const;
            bool operator!=(const BidirectionalListIterator<T>& it) const;
            bool operator<(const BidirectionalListIterator<T>& it) const;
            bool operator>(const BidirectionalListIterator<T>& it) const;
            bool operator>=(const BidirectionalListIterator<T>& it) const;
            bool operator<=(const BidirectionalListIterator<T>& it) const;

            operator BidirectionalListIterator<const T>() const;

        protected:
            BidirectionalListNode<T>* m_node;
    };

    template <typename T>
    class BidirectionalList {
        public:
            using iterator = BidirectionalListIterator<T>;
            using const_iterator = BidirectionalListIterator<const T>;
            using PointerTp = std::remove_pointer_t<std::remove_cvref_t<T>>*;
            using Node = BidirectionalListNode<T>*;
            using Allocator = PagedAllocator<BidirectionalListNode<T>, FixedAllocator<BidirectionalListNode<T>>>;
            
            BidirectionalList();
            ~BidirectionalList();

            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            
            T& back();
            const T& back() const;

            void push(const T& v);
            void insert(Node at, const T& v);
            void insert(Node at, const BidirectionalList<T>& v);
            void insert(Node at, std::initializer_list<T> list);
            void swap(Node a, Node b);

            T pop(bool fromFront = false);
            void remove(Node from, u32 count = 1);

            void clear();
            u32 size() const;

            BidirectionalList<T> concat(const BidirectionalList<T>& arr) const;
            BidirectionalList<T> concat(std::initializer_list<T> list) const;
            void append(const BidirectionalList<T>& arr);
            void append(std::initializer_list<T> list);

            T& operator[](u32 idx);
            const T& operator[](u32 idx) const;
            BidirectionalList<T>& operator=(const BidirectionalList<T>& rhs);
            BidirectionalList<T>& operator=(std::initializer_list<T> list);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*>> each(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T, Node>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T&, Node>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_v<F, T*, Node>> each(F&& cb);

            // element -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> each(F&& cb);

            // element, index -> do_break
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>> each(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T&, Node>> each(F&& cb);

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T*, Node>> each(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> some(F&& cb) const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, bool> some(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, BidirectionalList<T>> filter(F&& cb)  const;

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, BidirectionalList<T>> filter(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, PointerTp> find(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, const PointerTp> find(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, PointerTp> find(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const Node> findNode(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T>, Node> findNode(F&& cb);

            // element, index
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, const Node> findNode(F&& cb) const;

            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, Node>, Node> findNode(F&& cb);

            // element
            template <typename F, typename M = std::invoke_result_t<F, T>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T>, BidirectionalList<M>> map(F&& cb) const;

            // element, index
            template <typename F, typename M = std::invoke_result_t<F, T, u32>>
            std::enable_if_t<std::is_invocable_r_v<M, F, T, Node>, BidirectionalList<M>> map(F&& cb) const;

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> sort(F&& cb);

            // element
            template <typename F>
            std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, BidirectionalList<T>> sorted(F&& cb) const;

        protected:
            Allocator m_allocator;
            Node m_front;
            Node m_back;
            u32 m_size;
    };
};