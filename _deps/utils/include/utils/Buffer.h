#pragma once
#include <utils/Types.h>

namespace utils {
    class String;

    class Buffer {
        public:
            /** Creates a new dynamic buffer of default capacity (4096) */
            Buffer();
            /** Creates a new static buffer with a maximum capacity of {capacity} */
            Buffer(u64 capacity);
            ~Buffer();

            /** Writes {sz} bytes to buffer from {src} */
            bool write(const void* src, u64 sz);
            /** Reads {sz} bytes to {dst} from buffer */
            bool read(void* dst, u64 sz);
            /** Writes {sz} bytes to buffer from {src}. If {sz} is not passed, it defaults to {src->remaining()} */
            bool write(const Buffer* src, u64 sz = UINT64_MAX);
            /** Reads {sz} bytes to {dst} from buffer */
            bool read(Buffer* dst, u64 sz);
            /** Writes a string to the buffer */
            bool write(const String& src);
            /** Reads a string from the buffer, stopping at the first null terminator */
            String readStr();
            /** Reads a string of length {len} to {dst} from the buffer */
            String readStr(u32 len);
            /** Sets the current read/write position within the buffer. Must be > 0 and < capacity() */
            u64 position(u64 pos);
            /** Returns a pointer to the data in the buffer at position {offset}. Offset is from the beginning of the buffer. */
            void* data(u64 offset);
            /** Returns a pointer to the data in the buffer at position {offset}. Offset is from the beginning of the buffer. */
            const void* data(u64 offset) const;

            /** Whether or not the buffer can resize as data is written to it */
            bool canResize() const;
            /** The number of bytes currently stored in the buffer */
            u64 size() const;
            /** The number of bytes between the current read/write position and the end of the data in the buffer (size() - position()) */
            u64 remaining() const;
            /** The maximum number of bytes that can be stored in the buffer before it needs to be resized */
            u64 capacity() const;
            /** The current read/write position */
            u64 position() const;
            /** Whether or not the read/write position is at the end of the data in the buffer (position() == size()) */
            bool at_end() const;
            /** Returns a pointer to the data at the current read/write position */
            void* data();
            /** Returns a pointer to the data at the current read/write position */
            const void* data() const;
            /** Stores the data in the buffer in a file */
            bool save(const String& path) const;

            /** Writes some object to the buffer */
            template <typename T>
            bool write(const T& val);

            /** Reads some object from the buffer */
            template <typename T>
            bool read(T& val);

            static Buffer* FromFile(const String& path);
        
        protected:
            u8* m_data;
            u64 m_capacity;
            u64 m_pos;
            u64 m_used;
            bool m_canResize;
    };
};