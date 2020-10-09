#include <common/module.h>
#include <builtin/script_buffer.h>
#include <util/util.h>

#include <common/script_type.h>
#include <common/script_function.h>
#include <backends/backend.h>
#include <common/context.h>

namespace gjs {
    script_module::script_module(script_context* ctx, const std::string& file) : m_ctx(ctx), m_data(nullptr), m_init(nullptr) {
        m_types = new type_manager(ctx);
        size_t nameIdx = file.find_last_of('\\');
        if (nameIdx == std::string::npos) nameIdx = file.find_last_of('/');
        if (nameIdx == std::string::npos) nameIdx = 0;
        else nameIdx++;
        m_name = file.substr(nameIdx, file.find_last_of('.')  - nameIdx);
        m_id = hash(m_name);
        m_data = new script_buffer();
    }

    script_module::~script_module() {
        if (m_types) delete m_types;
        if (m_data) delete m_data;
        if (m_init) delete m_init;
    }

    void script_module::init() {
        if (!m_init) return;
        m_ctx->call<void>(m_init, nullptr);
    }

    void script_module::define_local(const std::string& name, u64 offset, script_type* type, const source_ref& ref) {
        if (has_local(name)) return;
        m_local_map[name] = (u16)m_locals.size();
        m_locals.push_back({ offset, type, name, ref });
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

    bool script_module::has_function(const std::string& name) {
        return m_func_map.count(name) > 0;
    }

    std::vector<script_function*> script_module::function_overloads(const std::string& name) {
        auto it = m_func_map.find(name);
        if (it == m_func_map.end()) return { };

        std::vector<script_function*> out;
        for (u32 i = 0;i < it->getSecond().size();i++) out.push_back(m_functions[it->getSecond()[i]]);
        return out;
    }

    script_function* script_module::function(const std::string& name) {
        if (name == "__init__") return m_init;

        auto it = m_func_map.find(name);
        if (it == m_func_map.end()) return nullptr;
        if (it->getSecond().size() > 1) return nullptr;
        return m_functions[it->getSecond()[0]];
    }

    std::vector<std::string> script_module::function_names() const {
        std::vector<std::string> out;
        for (auto i = m_func_map.begin();i != m_func_map.end();++i) {
            out.push_back(i->getFirst());
        }
        return out;
    }

    void script_module::add(script_function* func) {
        if (m_func_map.count(func->name) == 0) {
            m_func_map[func->name] = { (u32)m_functions.size() };
            m_functions.push_back(func);
        } else {
            std::vector<u32>& funcs = m_func_map[func->name];
            for (u8 i = 0;i < funcs.size();i++) {
                script_function* f = m_functions[funcs[i]];
                bool matches = f->signature.return_type->id() == func->signature.return_type->id();
                if (!matches) continue;

                if (f->signature.arg_types.size() != func->signature.arg_types.size()) continue;

                matches = true;
                for (u8 a = 0;a < f->signature.arg_types.size() && matches;a++) {
                    matches = (f->signature.arg_types[a]->id() == func->signature.arg_types[a]->id());
                }

                if (matches) {
                    throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
                }
            }

            funcs.push_back((u32)m_functions.size());
            m_functions.push_back(func);
        }

        func->owner = this;
        if (this != m_ctx->global()) m_ctx->add(func);
    }
};