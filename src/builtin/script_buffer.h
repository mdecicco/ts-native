#pragma once
#include <common/types.h>

namespace gjs {
    class script_string;

    /*
     * Initializing a buffer with 0 capacity will cause the buffer
     * to resize as needed. Use a fixed size if the memory address
     * of the data must not change.
     *
     * Resizable buffers are initialized with a capacity of 4kB to
     * start with.
     * 
     * Note: the 'size' (m_used) may not refer to the actual total
     * size of stored data, since the write position can be moved.
     * 'size' refers to the furthest written byte from the origin. 
     */
    class script_buffer {
        public:
            script_buffer();
            script_buffer(u64 capacity);
            ~script_buffer();

            void write(void* data, u64 sz);
            void read(void* out, u64 sz);
            void write(script_buffer* buf, u64 sz);
            void read(script_buffer* buf, u64 sz);
            void write(script_string* str);
            void read(script_string* str);
            void read(script_string* str, u32 len);
            u64 position(u64 pos);
            void* data(u64 offset);

            inline bool can_resize() const { return m_can_resize; }
            inline u64 size() const { return m_used; }
            inline u64 remaining() const { return m_used - m_position; }
            inline u64 capacity() const { return m_capacity; }
            inline u64 position() const { return m_position; }
            inline void* data() { return m_data + m_position; }
            inline bool at_end() const { return m_position == m_used; }

            template <typename T>
            inline void write(const T& val) { write(&val, sizeof(T)); }

            template <typename T>
            inline void read(T& val) { read(&val, sizeof(T)); }

        protected:
            u8* m_data;
            u64 m_position;
            bool m_can_resize;
            u64 m_used;
            u64 m_capacity;
    };
};
