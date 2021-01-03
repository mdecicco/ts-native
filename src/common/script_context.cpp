#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/common/io.h>
#include <gjs/backends/backend.h>
#include <gjs/backends/b_vm.h>
#include <gjs/vm/allocator.h>

#include <gjs/bind/bind.h>
#include <gjs/builtin/builtin.h>
#include <gjs/common/errors.h>

#include <filesystem>

namespace gjs {
    script_context::script_context(backend* generator, io_interface* io) : m_pipeline(this), m_backend(generator), m_host_call_vm(nullptr), m_io(io) {
        if (!m_backend) {
            m_owns_backend = true;
            m_backend = new vm_backend(new basic_malloc_allocator(), 8 * 1024 * 1024, 8 * 1024 * 1024);
        } else m_owns_backend = false;

        if (!m_io) {
            m_owns_io = true;
            m_io = new basic_io_interface();
        } else m_owns_io = false;
        
        m_global = new script_module(this, "__global__");
        add(m_global);
        m_host_call_vm = dcNewCallVM(4096);
        m_backend->m_ctx = this;

        init_context(this);
        m_backend->init();
    }

    script_context::~script_context() {
        for (auto it = m_modules.begin();it != m_modules.end();++it) delete it->getSecond();

        dcFree(m_host_call_vm);

        if (m_owns_backend) {
            basic_malloc_allocator* alloc = (basic_malloc_allocator*)((vm_backend*)m_backend)->allocator();
            delete m_backend;
            delete alloc;
        }

        if (m_owns_io) {
            delete m_io;
        }
    }

    void script_context::add(script_function* func) {
        u64 addr = 0;
        if (func->is_host) addr = (u64)func->access.wrapped->func_ptr;
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


    script_module* script_context::resolve(const std::string& module_path) {
        return resolve(m_io->get_cwd(), module_path);
    }

    script_module* script_context::resolve(const std::string& rel_path, const std::string& module_path) {
        // first check if it's a direct reference
        script_module* mod = module(module_path);
        if (mod) return mod;

        auto final_dir = split(rel_path, "/\\");
        if (!m_io->is_dir(rel_path)) {
            final_dir.pop_back();
        }

        auto mod_dirs = split(module_path, "/\\");

        for (u32 i = 0;i < mod_dirs.size() - 1;i++) {
            if (mod_dirs[i] == "..") {
                final_dir.pop_back();
                continue;
            }
            if (mod_dirs[i] == ".") continue;
            final_dir.push_back(mod_dirs[i]);
        }

        if (mod_dirs.back().find_last_of('.') == std::string::npos) {
            mod_dirs.back() += ".gjs";
        }
        final_dir.push_back(mod_dirs.back());

        std::string out_path = "";
        for (u32 i = 0;i < final_dir.size();i++) {
            if (i > 0) out_path += "/";
            out_path += final_dir[i];
        }

        // Make sure the path is absolute
        try {
            std::filesystem::path p(out_path);
            out_path = std::filesystem::absolute(p).u8string();
        } catch(...) {
            return nullptr;
        }

        if (!m_io->exists(out_path) || m_io->is_dir(out_path)) return nullptr;

        // check if the module was already loaded
        mod = module(out_path);
        if (mod) return mod;

        file_interface* file = m_io->open(out_path, io_open_mode::existing_only);
        if (file && file->size() > 0) {
            char* c_src = new char[file->size() + 1];
            c_src[file->size()] = 0;
            file->read(c_src, file->size());
            std::string src = c_src;
            delete [] c_src;
            m_io->close(file);

            return add_code(out_path, src);
        }

        return nullptr;
    }

    std::string script_context::module_name(const std::string& module_path) {
        return module_name(m_io->get_cwd(), module_path);
    }

    std::string script_context::module_name(const std::string& rel_path, const std::string& module_path) {
        // first check if it's a direct reference
        script_module* mod = module(module_path);
        if (mod) return mod->name();

        auto final_dir = split(rel_path, "/\\");
        if (!m_io->is_dir(rel_path)) {
            final_dir.pop_back();
        }

        auto mod_dirs = split(module_path, "/\\");

        for (u32 i = 0;i < mod_dirs.size() - 1;i++) {
            if (mod_dirs[i] == "..") {
                final_dir.pop_back();
                continue;
            }
            if (mod_dirs[i] == ".") continue;
            final_dir.push_back(mod_dirs[i]);
        }

        if (mod_dirs.back().find_last_of('.') == std::string::npos) {
            mod_dirs.back() += ".gjs";
        }
        final_dir.push_back(mod_dirs.back());

        std::string out_path = "";
        for (u32 i = 0;i < final_dir.size();i++) {
            if (i > 0) out_path += "/";
            out_path += final_dir[i];
        }

        // Make sure the path is absolute
        try {
            std::filesystem::path p(out_path);
            out_path = std::filesystem::absolute(p).u8string();
        } catch(...) {
            return "";
        }

        if (!m_io->exists(out_path) || m_io->is_dir(out_path)) return "";

        return out_path;
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

    script_module* script_context::add_code(const std::string& module, const std::string& code) {
        try {
            return m_pipeline.compile(module, code, m_backend);
        } catch (error::exception& e) {
            m_pipeline.log()->errors.push_back({ true, e.code, e.message, e.src });
        } catch (std::exception& e) {
            m_pipeline.log()->errors.push_back({ true, error::ecode::unspecified_error, std::string(e.what()), source_ref("[unknown]", "[unknown]", 0, 0) });
        }
        return false;
    }
};
