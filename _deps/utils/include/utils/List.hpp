#pragma once
#include <utils/List.h>
#include <utils/Allocator.hpp>

namespace utils {
    //
    // BidirectionalListIterator
    //

    template <typename T>
    BidirectionalListIterator<T>::BidirectionalListIterator(BidirectionalListNode<T>* node) : m_node(node) {
    }

    template <typename T>
    typename BidirectionalListIterator<T>::reference BidirectionalListIterator<T>::operator* () const {
        return m_node->value;
    }

    template <typename T>
    typename BidirectionalListIterator<T>::pointer BidirectionalListIterator<T>::operator->() const {
        return &m_node->value;
    }

    template <typename T>
    BidirectionalListIterator<T>& BidirectionalListIterator<T>::operator++() {
        m_node = m_node->m_next;
        return *this;
    }

    template <typename T>
    BidirectionalListIterator<T>& BidirectionalListIterator<T>::operator--() {
        m_node = m_node->m_prev;
        return *this;
    }

    template <typename T>
    typename BidirectionalListIterator<T>::difference_type BidirectionalListIterator<T>::operator-(const BidirectionalListIterator<T>& it) const {
        difference_type count = 0;
        BidirectionalListNode<T>* n = it.m_node;
        while (n) {
            count++;
            n = n->m_next;
        }
        return count;
    }

    template <typename T>
    BidirectionalListIterator<T> BidirectionalListIterator<T>::operator+(const difference_type& diff) const {
        BidirectionalListNode<T>* n = m_node;
        for (difference_type i = 0;i < diff;i++) n = n->m_next;
        return BidirectionalListIterator<T>(n);
    }

    template <typename T>
    BidirectionalListIterator<T> BidirectionalListIterator<T>::operator-(const difference_type& diff) const {
        BidirectionalListNode<T>* n = m_node;
        for (difference_type i = 0;i < diff;i++) n = n->m_prev;
        return BidirectionalListIterator<T>(n);
    }

    template <typename T>
    typename BidirectionalListIterator<T>::reference BidirectionalListIterator<T>::operator[] (const difference_type& offset) const {
        BidirectionalListNode<T>* n = m_node;
        for (difference_type i = 0;i < offset;i++) n = n->m_next;
        return BidirectionalListIterator<T>(n);
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator==(const BidirectionalListIterator<T>& it) const {
        return m_node == it.m_node;
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator!=(const BidirectionalListIterator<T>& it) const {
        return m_node != it.m_node;
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator<(const BidirectionalListIterator<T>& it) const {
        return m_node < it.m_node;
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator>(const BidirectionalListIterator<T>& it) const {
        return m_node > it.m_node;
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator>=(const BidirectionalListIterator<T>& it) const {
        return m_node >= it.m_node;
    }

    template <typename T>
    bool BidirectionalListIterator<T>::operator<=(const BidirectionalListIterator<T>& it) const {
        return m_node <= it.m_node;
    }

    template<typename T>
    BidirectionalListIterator<T>::operator BidirectionalListIterator<const T>() const {
        return BidirectionalListIterator<const T>(m_node);
    }



    //
    // BidirectionalList
    //

    template <typename T>
    BidirectionalList<T>::BidirectionalList() : m_allocator(128, 128), m_size(0), m_front(nullptr), m_back(nullptr) {
    }

    template <typename T>
    BidirectionalList<T>::~BidirectionalList() {
    }

    template <typename T>
    typename BidirectionalList<T>::iterator BidirectionalList<T>::begin() {
        return BidirectionalList<T>::iterator(m_front);
    }

    template <typename T>
    typename BidirectionalList<T>::const_iterator BidirectionalList<T>::begin() const {
        return BidirectionalList<T>::const_iterator(m_front);
    }

    template <typename T>
    typename BidirectionalList<T>::iterator BidirectionalList<T>::end() {
        return BidirectionalList<T>::iterator(nullptr);
    }

    template <typename T>
    typename BidirectionalList<T>::const_iterator BidirectionalList<T>::end() const {
        return BidirectionalList<T>::const_iterator(nullptr);
    }

    template <typename T>
    T& BidirectionalList<T>::back() {
        return m_back->value;
    }

    template <typename T>
    const T& BidirectionalList<T>::back() const {
        return m_back->value;
    }

    template <typename T>
    void BidirectionalList<T>::push(const T& v) {
        Node n = m_allocator.alloc(1);
        n->value = v;

        if (!m_back) {
            m_back = m_front = n;
            n->m_next = n->m_prev = nullptr;
        } else {
            m_back->m_next = n;
            n->m_prev = m_back;
            n->m_next = nullptr;
            m_back = n;
        }
        m_size++;
    }

    template <typename T>
    void BidirectionalList<T>::insert(Node at, const T& v) {
        if (m_size == 0) return;
        Node n = m_allocator.alloc(1);
        n->value = v;

        n->m_prev = at;
        n->m_next = at->m_next;
        if (at->m_next) at->m_next->m_prev = n;
        at->m_next = n;

        if (at == m_back) m_back = n;

        m_size++;
    }

    template <typename T>
    void BidirectionalList<T>::insert(Node at, const BidirectionalList<T>& v) {
        if (m_size == 0 || v.m_size == 0) return;

        Node tmp = at;
        Node first = nullptr;
        Node on = v.m_front;
        while (on) {
            Node n = m_allocator.alloc(1);
            if (!first) first = n;

            n->value = on->value;

            n->m_prev = at;
            n->m_next = at->m_next;
            if (at->m_next) at->m_next->m_prev = n;
            at->m_next = n;
            at = n;

            on = on->m_next;
        }

        if (tmp == m_back) m_back = first;
        
        m_size += v.m_size;
    }

    template <typename T>
    void BidirectionalList<T>::insert(Node at, std::initializer_list<T> list) {
        u32 sz = (u32)list.size();
        if (sz == 0) return;

        Node tmp = at;
        Node first = nullptr;

        for (auto ele : list) {
            Node n = m_allocator.alloc(1);
            if (!first) first = n;

            n->value = ele;

            n->m_prev = at;
            n->m_next = at->m_next;
            if (at->m_next) at->m_next->m_prev = n;
            at->m_next = n;
            at = n;
        }

        if (tmp == m_back) m_back = first;

        m_size += sz;
    }

    template <typename T>
    void BidirectionalList<T>::swap(Node a, Node b) {
        if (a == b) return;

        Node tmpPrev = a->m_prev;
        Node tmpNext = a->m_next;

        a->m_next = b->m_next;
        a->m_prev = b->m_prev;
        if (a->m_next) a->m_next->m_prev = a;
        if (a->m_prev) a->m_prev->m_next = a;

        b->m_next = tmpNext;
        b->m_prev = tmpPrev;
        if (b->m_next) b->m_next->m_prev = b;
        if (b->m_prev) b->m_prev->m_next = b;

        if (a == m_front) m_front = b;
        if (a == m_back) m_back = b;
        if (b == m_front) m_front = a;
        if (b == m_back) m_back = a;
    }

    template <typename T>
    T BidirectionalList<T>::pop(bool fromFront) {
        if (m_size == 0) {
            // todo exception
        }

        if (fromFront) {
            T result = m_front->value;
            Node tmp = m_front;
            m_front = m_front->m_next;
            m_front->m_prev = nullptr;
            m_allocator.free(tmp);
            m_size--;
            return result;
        }

        T result = m_back->value;
        Node tmp = m_back;
        m_back = m_back->m_prev;
        m_back->m_next = nullptr;
        m_allocator.free(tmp);
        m_size--;
        return result;
    }

    template <typename T>
    void BidirectionalList<T>::remove(Node from, u32 count) {
        if (!from || m_size == 0 || count == 0) return;
        
        Node begin = from;
        Node end = from;
        for (u32 i = 0;i < count && end;i++) end = end->m_next;

        // remove selected nodes from list
        if (end) end->m_prev = begin->m_prev;
        if (begin->m_prev) begin->m_prev->m_next = end;

        if (begin == m_front) m_front = end;
        if (!end) m_back = begin->m_prev;

        // free selected nodes
        while (begin && begin != end) {
            Node tmp = begin->m_next;
            m_allocator.free(begin);
            begin = tmp;
        }

        m_size -= count;
    }

    template <typename T>
    void BidirectionalList<T>::clear() {
        m_size = 0;
        m_allocator.reset();
    }

    template <typename T>
    u32 BidirectionalList<T>::size() const {
        return m_size;
    }

    template <typename T>
    BidirectionalList<T> BidirectionalList<T>::concat(const BidirectionalList<T>& o) const {
        if ((m_size + o.m_size) == 0) return BidirectionalList<T>();

        BidirectionalList<T> out(m_size + o.m_size);
        Node n = m_front;
        while (n) {
            out.push(n->value);
            n = n->m_next;
        }

        n = o.m_front;
        while (n) {
            out.push(n->value);
            n = n->m_next;
        }

        return out;
    }

    template <typename T>
    BidirectionalList<T> BidirectionalList<T>::concat(std::initializer_list<T> list) const {
        u32 sz = (u32)list.size();
        if ((m_size + sz) == 0) return BidirectionalList<T>();

        BidirectionalList<T> out(m_size + sz);
        Node n = m_front;
        while (n) {
            out.push(n->value);
            n = n->m_next;
        };

        for (auto ele : list) out.push(ele);

        return out;
    }

    template <typename T>
    void BidirectionalList<T>::append(const BidirectionalList<T>& o) {
        if (o.m_size == 0) return;

        Node n = o.m_front;
        while (n) {
            push(n->value);
            n = n->m_next;
        }
    }

    template <typename T>
    void BidirectionalList<T>::append(std::initializer_list<T> list) {
        u32 sz = (u32)list.size();
        if (sz == 0) return;
        for (auto ele : list) push(ele);
    }

    template <typename T>
    T& BidirectionalList<T>::operator[](u32 idx) {
        // todo exceptions
        Node n = m_front;
        for (u32 i = 0;i < idx && n;i++) n = n->m_next;
        return n->value;
    }

    template <typename T>
    const T& BidirectionalList<T>::operator[](u32 idx) const {
        // todo exceptions
        Node n = m_front;
        for (u32 i = 0;i < idx && n;i++) n = n->m_next;
        return n->value;
    }

    template <typename T>
    BidirectionalList<T>& BidirectionalList<T>::operator=(const BidirectionalList<T>& rhs) {
        clear();
        append(rhs);
        return *this;
    }

    template <typename T>
    BidirectionalList<T>& BidirectionalList<T>::operator=(std::initializer_list<T> list) {
        clear();
        append(list);
        return *this;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T>> BidirectionalList<T>::each(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(n->value);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(n->value);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(&n->value);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(n->value, n);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T&, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(n->value, n);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_v<F, T*, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            cb(&n->value, n);
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>> BidirectionalList<T>::each(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(&n->value)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T&, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T*, BidirectionalListNode<T>*>> BidirectionalList<T>::each(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(&n->value, n)) break;
        }
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, bool> BidirectionalList<T>::some(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, bool> BidirectionalList<T>::some(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) return true;
        }

        return false;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, BidirectionalList<T>> BidirectionalList<T>::filter(F&& cb) const {
        BidirectionalList<T> out;

        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) out.push(n->value);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, BidirectionalList<T>> BidirectionalList<T>::filter(F&& cb) const {
        BidirectionalList<T> out;

        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) out.push(n->value);
        }

        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const typename BidirectionalList<T>::PointerTp> BidirectionalList<T>::find(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) {
                if constexpr (std::is_pointer_v<T>) return n->value;
                else return &n->value;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, typename BidirectionalList<T>::PointerTp> BidirectionalList<T>::find(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) {
                if constexpr (std::is_pointer_v<T>) return n->value;
                else return &n->value;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, const typename BidirectionalList<T>::PointerTp> BidirectionalList<T>::find(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) {
                if constexpr (std::is_pointer_v<T>) return n->value;
                else return &n->value;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, typename BidirectionalList<T>::PointerTp> BidirectionalList<T>::find(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) {
                if constexpr (std::is_pointer_v<T>) return n->value;
                else return &n->value;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, const typename BidirectionalList<T>::Node> BidirectionalList<T>::findNode(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) {
                return n;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T>, typename BidirectionalList<T>::Node> BidirectionalList<T>::findNode(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value)) {
                return n;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, const typename BidirectionalList<T>::Node> BidirectionalList<T>::findNode(F&& cb) const {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) {
                return n;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, BidirectionalListNode<T>*>, typename BidirectionalList<T>::Node> BidirectionalList<T>::findNode(F&& cb) {
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            if (cb(n->value, n)) {
                return n;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T>, BidirectionalList<M>> BidirectionalList<T>::map(F&& cb) const {
        BidirectionalList<M> out(m_size);
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            out.push(cb(n->value));
        }
        return out;
    }

    template <typename T>
    template <typename F, typename M>
    std::enable_if_t<std::is_invocable_r_v<M, F, T, BidirectionalListNode<T>*>, BidirectionalList<M>> BidirectionalList<T>::map(F&& cb) const {
        BidirectionalList<M> out(m_size);
        Node n = m_front;
        for (Node n = m_front;n != nullptr;n = n->m_next) {
            out.push(cb(n->value, n));
        }
        return out;
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, void> BidirectionalList<T>::sort(F&& cb) {
        std::sort(begin(), end(), cb);
    }

    template <typename T>
    template <typename F>
    std::enable_if_t<std::is_invocable_r_v<bool, F, T, T>, BidirectionalList<T>> BidirectionalList<T>::sorted(F&& cb) const {
        BidirectionalList<T> out = *this;
        std::sort(out.begin(), out.end(), cb);
        return out;
    }
};