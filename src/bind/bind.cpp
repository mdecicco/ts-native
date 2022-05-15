#include <gs/bind/bind.h>

namespace gs {
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