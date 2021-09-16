#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/vm/register.h>
#include <gjs/bind/bind.h>
#include <gjs/util/util.h>

namespace gjs {
    script_function::script_function(script_context* ctx, const std::string _name, address addr, script_type* signature, script_type* method_of, script_module* mod) {
        m_ctx = ctx;

        name = _name;
        is_host = false;
        is_static = false;
        is_method_of = method_of;
        is_thiscall = false;
        is_subtype_obj_ctor = false;
        m_is_copy = false;
        type = signature;
        m_id = 0;

        access.entry = addr;

        owner = mod;
    }

    script_function::script_function(type_manager* mgr, script_type* tp, ffi::wrapped_function* wrapped, bool is_ctor, bool is_dtor, script_module* mod) {
        m_ctx = mgr->m_ctx;

        name = wrapped->name;
        is_host = true;
        is_static = wrapped->is_static_method;
        is_method_of = tp;
        is_thiscall = tp && !wrapped->is_static_method;
        is_subtype_obj_ctor = tp && tp->requires_subtype && is_ctor;
        m_is_copy = false;
        type = mgr->get(function_signature(mgr->ctx(), tp, wrapped, is_ctor), "", is_dtor);
        m_id = 0;
        access.wrapped = wrapped;
        owner = mod;

        if (!wrapped->is_anonymous) {
            if (!owner) owner = mgr->m_ctx->global();
            owner->add(this);
        }
    }

    bool substitute_subtype_in_sig(script_context* ctx, function_signature& sig, script_type* subtype, script_type* subtypet) {
        bool hasChanges = false;

        if (sig.return_type == subtypet) {
            sig.return_type = subtype;
            hasChanges = true;
        }

        for (u8 a = 0;a < sig.args.size();a++) {
            if (sig.args[a].tp == subtypet) {
                sig.args[a].tp = subtype;
                hasChanges = true;
                continue;
            }

            if (sig.args[a].tp->signature) {
                function_signature* asig = sig.args[a].tp->signature;
                function_signature newSig(*asig);
                bool didChange = substitute_subtype_in_sig(ctx, newSig, subtype, subtypet);
                if (didChange) {
                    hasChanges = true;
                    sig.args[a].tp = ctx->types()->get(newSig);
                }
            }
        }

        return hasChanges;
    }

    script_function* script_function::duplicate_with_subtype(script_type* subtype) {
        function_signature newSig(m_ctx, is_method_of, access.wrapped, is_subtype_obj_ctor);
        script_type* subtypet = type_of<subtype_t>(m_ctx);
        if (!substitute_subtype_in_sig(m_ctx, newSig, subtype, subtypet)) return this;

        // check to see if this function was already duplicated with the subtype
        script_function* existing = function_search(name, m_ctx->functions(), newSig);
        if (existing) return existing;

        script_type* sigTp = m_ctx->types()->get(newSig);
        script_function* copy = new script_function(m_ctx, name, 0, sigTp, is_method_of, owner);
        copy->is_host = true;
        copy->is_static = is_static;
        copy->is_method_of = is_method_of;
        copy->is_thiscall = is_thiscall;
        copy->is_subtype_obj_ctor = is_subtype_obj_ctor;
        copy->access.wrapped = access.wrapped;
        copy->m_is_copy = true;
        owner->add(copy);
        return copy;
    }

    script_context* script_function::ctx() const {
        return m_ctx;
    }

    function_id script_function::id() const {
        return m_id;
    }

    bool script_function::is_clone() const {
        return m_is_copy;
    }
};