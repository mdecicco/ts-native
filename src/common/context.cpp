#include <common/context.h>
#include <common/script_function.h>
#include <common/script_type.h>
#include <backends/backend.h>

#include <bind/bind.h>
#include <bind/builtin.h>
#include <common/errors.h>

namespace gjs {
    script_context::script_context(backend* generator) : 
         m_types(new type_manager(this)), m_pipeline(this), m_backend(generator)
    {
        m_backend->m_ctx = this;

        init_context(this);
    }

    script_context::~script_context() {
    }

    void script_context::add(script_function* func) {
        u64 addr = 0;
        if (func->is_host) addr = func->access.wrapped->address;
        else addr = func->access.entry;

        if (m_funcs_by_addr.count(addr) > 0) {
            throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
        }

        m_funcs_by_addr[addr] = func;

        if (m_funcs.count(func->name) == 0) {
            m_funcs[func->name] = { func };
        } else {
            std::vector<script_function*>& funcs = m_funcs[func->name];
            for (u8 i = 0;i < funcs.size();i++) {
                bool matches = funcs[i]->signature.return_type->id() == func->signature.return_type->id();
                if (!matches) continue;

                if (funcs[i]->signature.arg_types.size() != func->signature.arg_types.size()) continue;

                matches = true;
                for (u8 a = 0;a < funcs[i]->signature.arg_types.size() && matches;a++) {
                    matches = (funcs[i]->signature.arg_types[a]->id() == func->signature.arg_types[a]->id());
                }

                if (matches) {
                    throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
                }
            }

            funcs.push_back(func);
        }
    }

    script_function* script_context::function(const std::string& name) {
        auto it = m_funcs.find(name);
        if (it == m_funcs.end()) return nullptr;
        return it->getSecond()[0];
    }

    script_function* script_context::function(u64 address) {
        auto it = m_funcs_by_addr.find(address);
        if (it == m_funcs_by_addr.end()) return nullptr;
        return it->getSecond();
    }

    std::vector<script_function*> script_context::all_functions() {
        std::vector<script_function*> out;
        for (auto i = m_funcs.begin();i != m_funcs.end();++i) {
            out.insert(out.begin(), i->getSecond().begin(), i->getSecond().end());
        }
        return out;
    }

    std::vector<script_type*> script_context::all_types() {
        return m_types->all();
    }

    bool script_context::add_code(const std::string& filename, const std::string& code) {
        try {
            return m_pipeline.compile(filename, code, m_backend);
        } catch (error::exception& e) {
            m_pipeline.log()->errors.push_back({ true, e.code, e.message, e.src });
        } catch (std::exception& e) {
            m_pipeline.log()->errors.push_back({ true, error::ecode::unspecified_error, std::string(e.what()), source_ref("[unknown]", "[unknown]", 0, 0) });
        }
        return false;
    }
};
