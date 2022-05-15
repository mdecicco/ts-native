#include <gs/interfaces/IContextual.h>

namespace gs {
    IContextual::IContextual(Context* ctx) : m_ctx(ctx) {}
    IContextual::~IContextual() {}

    Context* IContextual::getContext() const {
        return m_ctx;
    }
};