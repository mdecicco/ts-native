#include <gjs/common/type_manager.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_function.h>
#include <gjs/bind/ffi.h>
#include <gjs/util/util.h>

namespace gjs {
    type_manager::type_manager(script_context* ctx) : m_ctx(ctx) {
    }

    type_manager::~type_manager() {
    }

    void type_manager::merge(type_manager* new_types) {
        auto all = new_types->all();
        new_types->m_types.clear();
        new_types->m_types_by_id.clear();

        for (u16 i = 0;i < all.size();i++) {
            if (this != all[i]->owner->types()) {
                all[i]->owner->types()->add(all[i]);
                continue;
            }

            if (m_types.count(all[i]->name) > 0) {
                throw error::bind_exception(error::ecode::b_type_already_bound, all[i]->name.c_str());
            }
            if (m_types_by_id.count(all[i]->id()) > 0) {
                throw error::bind_exception(error::ecode::b_type_hash_collision, all[i]->name.c_str());
            }

            m_types[all[i]->name] = all[i];
            m_types_by_id[all[i]->id()] = all[i];
        }
    }

    script_type* type_manager::get(const std::string& name) {
        if (m_types.count(name) == 0) return nullptr;
        return m_types[name];
    }

    script_type* type_manager::get(u32 id) {
        if (m_types_by_id.count(id) == 0) return nullptr;
        return m_types_by_id[id];
    }

    script_type* type_manager::get(const function_signature& sig, const std::string& name, bool is_dtor) {
        if (this != m_ctx->types()) return m_ctx->types()->get(sig, name);

        if (name.length() > 0) {
            if (m_types.count(name) == 0) {
                std::string n = sig.to_string();
                script_type* t = new script_type();
                t->m_id = hash(n);
                t->name = name;
                t->internal_name = n;
                t->signature = new function_signature(sig);
                t->size = sizeof(void*);
                t->is_pod = false;
                t->is_primitive = false;
                t->is_host = true;
                t->is_floating_point = false;
                t->is_trivially_copyable = false;
                t->is_unsigned = false;
                t->is_builtin = true;
                m_types[name] = t;
                m_types_by_id[t->m_id] = t;

                if (!is_dtor) {
                    void (*ptr)(void*) = reinterpret_cast<void(*)(void*)>(raw_callback::destroy);
                    auto wrapped = ffi::wrap<void, void*>(this, name + "::destructor", ptr);
                    wrapped->arg_is_ptr.clear();
                    wrapped->arg_types.clear();
                    t->destructor = new script_function(this, t, wrapped, false, true, m_ctx->global());
                }

                return t;
            }

            script_type* t = m_types[name];
            if (!t->destructor) {
                void (*ptr)(void*) = reinterpret_cast<void(*)(void*)>(raw_callback::destroy);
                auto wrapped = ffi::wrap<void, void*>(this, name + "::destructor", ptr);
                wrapped->arg_is_ptr.clear();
                wrapped->arg_types.clear();
                t->destructor = new script_function(this, t, wrapped, false, true, m_ctx->global());
            }
            return t;
        }

        std::string n = sig.to_string();
        if (m_types.count(n) == 0) {
            script_type* t = m_ctx->types()->get(n);
            if (t) {
                if (!t->destructor) {
                    void (*ptr)(void*) = reinterpret_cast<void(*)(void*)>(raw_callback::destroy);
                    auto wrapped = ffi::wrap<void, void*>(this, n + "::destructor", ptr);
                    wrapped->arg_is_ptr.clear();
                    wrapped->arg_types.clear();
                    t->destructor = new script_function(this, t, wrapped, false, true, m_ctx->global());
                }
                return t;
            }

            t = new script_type();
            t->m_id = hash(n);
            t->name = n;
            t->internal_name = n;
            t->signature = new function_signature(sig);
            t->size = sizeof(void*);
            t->is_pod = false;
            t->is_primitive = false;
            t->is_host = true;
            t->is_floating_point = false;
            t->is_trivially_copyable = false;
            t->is_unsigned = false;
            t->is_builtin = true;
            m_types[n] = t;
            m_types_by_id[t->m_id] = t;

            if (!is_dtor) {
                void (*ptr)(void*) = reinterpret_cast<void(*)(void*)>(raw_callback::destroy);
                auto wrapped = ffi::wrap<void, void*>(this, n + "::destructor", ptr);
                wrapped->arg_is_ptr.clear();
                wrapped->arg_types.clear();
                t->destructor = new script_function(this, t, wrapped, false, true, m_ctx->global());
            }

            return t;
        }

        return m_types[n];
    }

    script_type* type_manager::add(const std::string& name, const std::string& internal_name) {
        if (m_types.count(internal_name) > 0) {
            throw error::bind_exception(error::ecode::b_type_already_bound, name.c_str());
        }
        u32 id = hash(name);
        if (m_types_by_id.count(id) > 0) {
            throw error::bind_exception(error::ecode::b_type_hash_collision, name.c_str());
        }

        script_type* t = new script_type();
        t->m_id = id;
        t->name = name;
        t->internal_name = internal_name;

        m_types[internal_name] = t;
        m_types_by_id[id] = t;

        m_ctx->types()->add(t);

        return t;
    }
    
    void type_manager::add(script_type* type) {
        if (m_types.count(type->internal_name) > 0) {
            throw error::bind_exception(error::ecode::b_type_already_bound, type->name.c_str());
        }

        if (m_types_by_id.count(type->id()) > 0) {
            throw error::bind_exception(error::ecode::b_type_hash_collision, type->name.c_str());
        }

        m_types[type->internal_name] = type;
        m_types_by_id[type->m_id] = type;
    }

    script_type* type_manager::finalize_class(ffi::wrapped_class* wrapped, script_module* mod) {
        auto it = m_types.find(wrapped->internal_name);
        if (it == m_types.end()) {
            throw error::bind_exception(error::ecode::b_type_not_found_cannot_finalize, wrapped->name.c_str());
        }

        if (wrapped->dtor) {
            // gjs considers constructors and destructors methods, but they are
            // bound like regular C functions. Remove explicit 'this' argument
            // because gjs implicitly adds it.
            wrapped->dtor->arg_types.erase(wrapped->dtor->arg_types.begin());
            wrapped->dtor->arg_is_ptr.erase(wrapped->dtor->arg_is_ptr.begin());
        }

        script_type* t = it->getSecond();
        t->is_pod = wrapped->is_pod;
        t->m_wrapped = wrapped;
        t->is_trivially_copyable = wrapped->trivially_copyable;
        t->requires_subtype = false;
        t->size = (u32)wrapped->size;
        t->destructor = wrapped->dtor ? new script_function(this, t, wrapped->dtor, false, true, mod) : nullptr;

        for (auto i = wrapped->properties.begin();i != wrapped->properties.end();++i) {
            t->properties.push_back({
                i->getSecond()->flags,
                i->getFirst(),
                i->getSecond()->type,
                i->getSecond()->offset,
                i->getSecond()->getter ? new script_function(this, t, i->getSecond()->getter, false, false, mod) : nullptr,
                i->getSecond()->setter ? new script_function(this, t, i->getSecond()->setter, false, false, mod) : nullptr
            });

            if (i->getSecond()->type->name == "subtype") {
                t->requires_subtype = true;
            }
        }

        for (u32 i = 0;i < wrapped->methods.size();i++) {
            ffi::wrapped_function* f = wrapped->methods[i];
            for (u8 a = 0;a < f->arg_types.size();a++) {
                if (f->arg_types[a]->name == "subtype") {
                    t->requires_subtype = true;
                }
            }
        }

        for (u32 i = 0;i < wrapped->methods.size();i++) {
            ffi::wrapped_function* f = wrapped->methods[i];
            if (f->name.find("::constructor") != std::string::npos) {
                // gjs considers constructors and destructors methods, but they are
                // bound like regular C functions. Remove explicit 'this' argument
                // because gjs implicitly adds it.
                f->arg_types.erase(f->arg_types.begin());
                f->arg_is_ptr.erase(f->arg_is_ptr.begin());

                if (t->requires_subtype) {
                    // second argument is a moduletype id, but should not be
                    // explicitly listed as an argument. gjs deals with passing
                    // that parameter internally
                    f->arg_is_ptr.erase(f->arg_is_ptr.begin());
                    f->arg_types.erase(f->arg_types.begin());
                }

                t->methods.push_back(new script_function(this, t, f, true, false, mod));
                continue;
            }
            t->methods.push_back(new script_function(this, t, f, false, false, mod));
        }

        t->owner = mod;
        return t;
    }

    std::vector<script_type*> type_manager::all() {
        std::vector<script_type*> out;
        for (auto i = m_types.begin();i != m_types.end();++i) {
            out.push_back(i->getSecond());
        }
        return out;
    }

    std::vector<script_type*> type_manager::exported() {
        std::vector<script_type*> out;
        for (auto i = m_types.begin();i != m_types.end();++i) {
            if (i->getSecond()->is_exported) out.push_back(i->getSecond());
        }
        return out;
    }

    script_context* type_manager::ctx() {
        return m_ctx;
    }
};