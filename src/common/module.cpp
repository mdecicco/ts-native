#include <common/module.h>
#include <builtin/script_buffer.h>
#include <util/util.h>

namespace gjs {
    script_module::script_module(script_context* ctx, const std::string& file) : m_ctx(ctx), m_data(nullptr) {
        size_t nameIdx = file.find_last_of('\\');
        if (nameIdx == std::string::npos) nameIdx = file.find_last_of('/');
        if (nameIdx == std::string::npos) nameIdx = 0;
        m_name = file.substr(nameIdx, file.find_last_of('.')  - nameIdx);
        m_id = hash(m_name);
        m_data = new script_buffer();
    }

    script_module::~script_module() {
        if (m_data) delete m_data;
        m_data = nullptr;
    }

    bool script_module::has_local(const std::string& name) const {
        return m_local_map.count(name) > 0;
    }

    const script_module::local_var& script_module::local(const std::string& name) const {
        static script_module::local_var invalid = { u64(-1), nullptr, "" };
        auto it = m_local_map.find(name);
        if (it == m_local_map.end()) return invalid;
        return m_locals[it->getSecond()];
    }
};