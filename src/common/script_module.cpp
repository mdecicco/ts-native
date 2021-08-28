#include <gjs/common/script_module.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_enum.h>
#include <gjs/common/script_object.h>

#include <gjs/builtin/script_buffer.h>
#include <gjs/util/util.h>
#include <gjs/backends/backend.h>

namespace gjs {
    script_module::script_module(script_context* ctx, const std::string& name, const std::string& path) : m_ctx(ctx), m_data(nullptr), m_init(nullptr) {
        m_types = new type_manager(ctx);
        m_name = name;
        m_id = hash(path);
        m_data = new script_buffer();
        m_initialized = false;
    }

    script_module::~script_module() {
        if (m_initialized) {
            for (u32 i = 0;i < m_locals.size();i++) {
                auto& l = m_locals[i];
                if (l.type->destructor) {
                    l.type->destructor->call(m_data->data(l.offset));
                }
            }
        }

        if (m_types) delete m_types;
        if (m_data) delete m_data;
        if (m_init) delete m_init;
    }

    void script_module::init() {
        if (!m_init || m_initialized) return;
        m_initialized = true;
        m_ctx->call(m_init, nullptr);
    }

    script_object script_module::define_local(const std::string& name, script_type* type) {
        if (has_local(name)) {
            // todo: exception
            return script_object(m_ctx);
        }
        m_local_map[name] = (u16)m_locals.size();
        u64 offset = m_data->position();
        m_locals.push_back({ offset, type, name, source_ref(), this });
        m_data->position(offset + type->size);
        return script_object(const_cast<script_context*>(m_ctx), const_cast<script_module*>(this), type, (u8*)m_data->data(offset));
    }

    void script_module::define_local(const std::string& name, u64 offset, script_type* type, const source_ref& ref) {
        if (has_local(name)) {
            // todo: exception
            return;
        }
        m_local_map[name] = (u16)m_locals.size();
        m_locals.push_back({ offset, type, name, ref, this });
    }

    bool script_module::has_local(const std::string& name) const {
        return m_local_map.count(name) > 0;
    }

    const script_module::local_var& script_module::local_info(const std::string& name) const {
        static script_module::local_var invalid = { u64(-1), nullptr, "" };
        auto it = m_local_map.find(name);
        if (it == m_local_map.end()) return invalid;
        return m_locals[it->getSecond()];
    }

    script_object script_module::local(const std::string& name) const {
        static script_object invalid = script_object((script_context*)nullptr, (script_module*)nullptr, (script_type*)nullptr, (u8*)nullptr);
        auto it = m_local_map.find(name);
        if (it == m_local_map.end()) return invalid;
        auto& l = m_locals[it->getSecond()];
        return script_object(const_cast<script_context*>(m_ctx), const_cast<script_module*>(this), l.type, (u8*)m_data->data(l.offset));
    }

    void* script_module::local_ptr(const std::string& name) const {
        auto it = m_local_map.find(name);
        if (it == m_local_map.end()) return nullptr;
        return m_data->data(m_locals[it->getSecond()].offset);
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

    script_type* script_module::type(const std::string& name) const {
        return m_types->get(name);
    }

    std::vector<std::string> script_module::function_names() const {
        std::vector<std::string> out;
        for (auto i = m_func_map.begin();i != m_func_map.end();++i) {
            out.push_back(i->getFirst());
        }
        return out;
    }

    script_enum* script_module::get_enum(const std::string& name) const {
        for (u32 i = 0;i < m_enums.size();i++) {
            if (m_enums[i]->name() == name) return m_enums[i];
        }
    }

    void script_module::add(script_function* func) {
        if (m_func_map.count(func->name) == 0) {
            m_func_map[func->name] = { (u32)m_functions.size() };
            m_functions.push_back(func);
        } else {
            std::vector<u32>& funcs = m_func_map[func->name];
            for (u8 i = 0;i < funcs.size();i++) {
                script_function* f = m_functions[funcs[i]];
                bool matches = f->type->signature->return_type->id() == func->type->signature->return_type->id();
                if (!matches) continue;

                if (f->type->signature->explicit_argc != func->type->signature->explicit_argc) continue;

                matches = true;
                for (u8 a = 0;a < f->type->signature->explicit_argc && matches;a++) {
                    matches = (f->type->signature->explicit_arg(a).tp->id() == func->type->signature->explicit_arg(a).tp->id());
                }

                if (matches) {
                    throw error::bind_exception(error::ecode::b_function_already_added_to_module, func->name.c_str(), m_name.c_str());
                }
            }

            funcs.push_back((u32)m_functions.size());
            m_functions.push_back(func);
        }

        func->owner = this;
        m_ctx->add(func);
    }
};