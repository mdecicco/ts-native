#include <common/context.h>
#include <common/script_function.h>
#include <common/script_type.h>
#include <common/module.h>
#include <backends/backend.h>

#include <bind/bind.h>
#include <builtin/builtin.h>
#include <common/errors.h>

namespace gjs {
    script_context::script_context(backend* generator) : m_pipeline(this), m_backend(generator), m_host_call_vm(nullptr) {
        m_global = new script_module(this, "__global__");
        add(m_global);
        m_host_call_vm = dcNewCallVM(4096);
        m_backend->m_ctx = this;

        init_context(this);
    }

    script_context::~script_context() {
        dcFree(m_host_call_vm);
        delete m_global;
    }

    void script_context::add(script_function* func) {
        u64 addr = 0;
        if (func->is_host) addr = func->access.wrapped->address;
        else addr = func->access.entry;

        if (m_funcs_by_addr.count(addr) > 0) {
            throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
        }

        m_funcs_by_addr[addr] = func;
        if (!func->owner) m_global->add(func);
    }

    void script_context::add(script_module* module) {
        if (m_modules.count(module->name()) > 0) {
            throw bind_exception(format("Module '%s' has already been added to the context", module->name().c_str()));
        }

        if (m_modules_by_id.count(module->id()) > 0) {
            throw bind_exception(format("Hash collision binding module '%s' (id: %lu)", module->name().c_str(), module->id()));
        }

        m_modules[module->name()] = module;
        m_modules_by_id[module->id()] = module;
    }

    script_module* script_context::resolve(const std::string& rel_path, const std::string& module_path) {
        // first check if it's a direct reference
        script_module* mod = module(module_path);
        if (mod) return mod;

        return nullptr;
    }

    script_module* script_context::module(const std::string& name) {
        auto it = m_modules.find(name);
        if (it == m_modules.end()) return nullptr;
        return it->getSecond();
    }

    script_module* script_context::module(u32 id) {
        auto it = m_modules_by_id.find(id);
        if (it == m_modules_by_id.end()) return nullptr;
        return it->getSecond();
    }

    std::vector<script_module*> script_context::modules() const {
        std::vector<script_module*> out;
        
        for (auto it = m_modules_by_id.begin();it != m_modules_by_id.end();++it) {
            out.push_back(it->getSecond());
        }

        return out;
    }

    script_function* script_context::function(u64 address) {
        auto it = m_funcs_by_addr.find(address);
        if (it == m_funcs_by_addr.end()) return nullptr;
        return it->getSecond();
    }

    script_module* script_context::add_code(const std::string& filename, const std::string& code) {
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
