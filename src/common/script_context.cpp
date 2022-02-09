#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_module.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/io.h>
#include <gjs/backends/backend.h>
#include <gjs/backends/b_vm.h>
#include <gjs/vm/allocator.h>
#include <gjs/util/util.h>

#include <gjs/bind/bind.h>
#include <gjs/builtin/builtin.h>
#include <gjs/builtin/script_process.h>
#include <gjs/common/errors.h>

#include <filesystem>

namespace gjs {
    script_context::script_context(backend* generator, io_interface* io) : m_pipeline(nullptr), m_backend(generator), m_host_call_vm(nullptr), m_io(io) {
        if (!m_backend) {
            m_owns_backend = true;
            m_backend = new vm_backend(new basic_malloc_allocator(), 8 * 1024 * 1024, 8 * 1024 * 1024);
        } else m_owns_backend = false;

        if (!m_io) {
            m_owns_io = true;
            m_io = new basic_io_interface();
        } else m_owns_io = false;

        m_pipeline = new pipeline(this);

        m_all_types = new type_manager(this);
        
        m_global = create_module("__global__", "builtin/__global__");
        m_host_call_vm = dcNewCallVM(4096);
        m_backend->m_ctx = this;

        init_context(this);

        m_backend->init();
    }

    script_context::script_context(u32 argc, const char** argv, backend* generator, io_interface* io) : script_context(generator, io) {
        u64 off = m_global->data()->position();
        void* ptr = m_global->data()->data();
        new (ptr) script_process(argc, argv);

        m_global->define_local("process", off, m_all_types->get<script_process>(), source_ref(), true);
        m_global->data()->position(off + sizeof(script_process));
    }

    script_context::~script_context() {
        for (auto it = m_modules.begin();it != m_modules.end();++it) delete it->getSecond();

        delete m_pipeline;

        dcFree(m_host_call_vm);

        if (m_owns_backend) {
            basic_malloc_allocator* alloc = (basic_malloc_allocator*)((vm_backend*)m_backend)->allocator();
            delete m_backend;
            delete alloc;
        }

        if (m_owns_io) {
            delete m_io;
        }

        delete m_all_types;
    }

    void script_context::add(script_function* func) {
        if (func->m_id > 0) {
            throw error::bind_exception(error::ecode::b_function_already_bound, func->name.c_str());
        }

        m_funcs.push_back(func);
        func->m_id = m_funcs.size();
    }

    void script_context::add(script_module* module) {
        if (m_modules.count(module->name()) > 0) {
            throw error::bind_exception(error::ecode::b_module_already_created, module->name().c_str());
        }

        if (m_modules_by_id.count(module->id()) > 0) {
            throw error::bind_exception(error::ecode::b_module_hash_collision, module->name().c_str(), module->id());
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
        // try {
            std::filesystem::path p(out_path);
            out_path = std::filesystem::absolute(p).u8string();
        // } catch(const std::exception& e) {
        //     return nullptr;
        // }

        if (!m_io->exists(out_path) || m_io->is_dir(out_path)) {
            throw error::runtime_exception(error::ecode::r_file_not_found, out_path.c_str());
        }

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

            std::string name = std::filesystem::path(out_path).filename().u8string();
            u8 extIdx = (u8)name.find_last_of('.');
            if (extIdx != std::string::npos) name = name.substr(0, extIdx);

            return add_code(name, out_path, src);
        }

        throw error::runtime_exception(error::ecode::r_failed_to_open_file, out_path.c_str());
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

    script_module* script_context::create_module(const std::string& name, const std::string& path) {
        script_module* m = new script_module(this, name, path);
        add(m);
        return m;
    }
    void script_context::destroy_module(script_module* mod) {
        if (mod->context() != this) return;

        auto types = mod->types()->all();
        auto funcs = mod->functions();
        auto enums = mod->enums();

        m_modules.erase(mod->m_name);
        m_modules_by_id.erase(mod->m_id);
        delete mod;

        for (auto tp : types) {
            m_all_types->remove(tp);
            delete tp;
        }

        for (auto func : funcs) {
            m_funcs[func->m_id - 1] = nullptr;
            delete func;
        }

        for (auto _enum : enums) {
            delete _enum;
        }
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
    const std::vector<script_function*>& script_context::functions() const {
        return m_funcs;
    }

    script_function* script_context::function(function_id id) {
        if (id == 0 || id > m_funcs.size()) return nullptr;
        return m_funcs[id - 1];
    }

    script_module* script_context::add_code(const std::string& module_name, const std::string& module_path, const std::string& code) {
        try {
            return m_pipeline->compile(module_name, module_path, code, m_backend);
        } catch (error::exception& e) {
            m_pipeline->log()->errors.push_back({ true, e.code, e.message, e.src });
        } catch (std::exception& e) {
            m_pipeline->log()->errors.push_back({ true, error::ecode::unspecified_error, std::string(e.what()), source_ref("[unknown]", "[unknown]", 0, 0) });
        }
        return nullptr;
    }


    script_context* script_context::current() {
        return current_ctx();
    }
    
    script_type* script_context::type(u64 moduletype_id) {
        script_context* ctx = current_ctx();
        script_module* mod = ctx->module(extract_left_u32(moduletype_id));
        if (!mod) return nullptr;
        return mod->types()->get(extract_right_u32(moduletype_id));
    }
};
