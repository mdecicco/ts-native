#include <utils/Buffer.h>

namespace utils {
    template <typename T>
    bool Buffer::write(const T& val) {
        return write(&val, sizeof(T));
    }

    template <typename T>
    bool Buffer::read(T& val) {
        return read(&val, sizeof(T));
    }
};