#include <tsn/interfaces/IContextual.h>

namespace tsn {
    IContextual::IContextual(Context* ctx) : m_ctx(ctx) {}
    IContextual::~IContextual() {}

    Context* IContextual::getContext() const {
        return m_ctx;
    }
};