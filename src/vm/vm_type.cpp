#include <vm/vm_function.h>
#include <vm/vm_type.h>
#include <util.h>
#include <bind.h>

namespace gjs {
    type_manager::type_manager(vm_context* ctx) : m_ctx(ctx) {
    }

    type_manager::~type_manager() {
    }

    vm_type* type_manager::get(const std::string& name) {
        if (m_types.count(name) == 0) return nullptr;
        return m_types[name];
    }

    vm_type* type_manager::get(u32 id) {
        if (m_types_by_id.count(id) == 0) return nullptr;
        return m_types_by_id[id];
    }

    vm_type* type_manager::add(const std::string& name, const std::string& internal_name) {
        if (m_types.count(internal_name) > 0) {
            throw bind_exception(format("Type '%s' already bound", name.c_str()));
        }
        u32 id = hash(name);
        if (m_types_by_id.count(id) > 0) {
            throw bind_exception(format("There was a type id collision while binding type '%s'", name.c_str()));
        }

        vm_type* t = new vm_type();
        t->m_id = id;
        t->name = name;
        t->internal_name = internal_name;

        m_types[internal_name] = t;
        m_types_by_id[id] = t;

        return t;
    }

    vm_type* type_manager::finalize_class(bind::wrapped_class* wrapped) {
        auto it = m_types.find(wrapped->internal_name);
        if (it == m_types.end()) {
            throw bind_exception(format("Type '%s' not found and can not be finalized", wrapped->name.c_str()));
        }

        vm_type* t = it->getSecond();
        t->m_wrapped = wrapped;
        t->requires_subtype = wrapped->requires_subtype;
        t->size = wrapped->size;
        t->destructor = wrapped->dtor ? new vm_function(this, t, wrapped->dtor, false, true) : nullptr;

        for (auto i = wrapped->properties.begin();i != wrapped->properties.end();++i) {
            t->properties.push_back({
                i->getSecond()->flags,
                i->getFirst(),
                get(i->getSecond()->type.name()),
                i->getSecond()->offset,
                i->getSecond()->getter ? new vm_function(this, t, i->getSecond()->getter) : nullptr,
                i->getSecond()->setter ? new vm_function(this, t, i->getSecond()->setter) : nullptr
            });
        }

        for (u32 i = 0;i < wrapped->methods.size();i++) {
            bind::wrapped_function* f = wrapped->methods[i];
            if (f->name.find("::constructor") != std::string::npos) {
                t->methods.push_back(new vm_function(this, t, f, true));
                continue;
            }
            t->methods.push_back(new vm_function(this, t, f));
        }

        return t;
    }

    std::vector<vm_type*> type_manager::all() {
        std::vector<vm_type*> out;
        for (auto i = m_types.begin();i != m_types.end();++i) {
            out.push_back(i->getSecond());
        }
        return out;
    }


    vm_type::vm_type() {
        destructor = nullptr;
        size = 0;
        is_primitive = false;
        is_floating_point = false;
        is_unsigned = false;
        is_builtin = false;
        is_host = false;
        requires_subtype = false;
        m_wrapped = nullptr;
        base_type = nullptr;
        sub_type = nullptr;
    }

    vm_type::~vm_type() {
        if (m_wrapped) delete m_wrapped;
    }
};