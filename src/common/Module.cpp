#include <gs/common/Module.h>

namespace gs {
    Module::Module(Context* ctx, const utils::String& name) : IContextual(ctx), m_name(name) {}
    Module::~Module() {}

    const utils::String& Module::getName() const {
        return m_name;
    }
};