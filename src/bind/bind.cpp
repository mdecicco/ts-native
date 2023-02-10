#include <tsn/bind/bind.h>

namespace tsn {
    namespace ffi {
        BindException::BindException(const utils::String& message) : m_message(message) {
        }

        BindException::~BindException() {
        }

        char const* BindException::what() const {
            return m_message.c_str();
        }
    };
};