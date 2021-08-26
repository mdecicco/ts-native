#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/vm/register.h>
#include <gjs/bind/bind.h>
#include <gjs/util/util.h>

namespace gjs {
    script_function::script_function(script_context* ctx, const std::string _name, address addr, script_type* signature, script_module* mod) {
        m_ctx = ctx;

        name = _name;
        is_host = false;
        is_static = false;
        is_method_of = nullptr;
        is_thiscall = false;
        is_subtype_obj_ctor = false;
        type = signature;

        access.entry = addr;

        owner = mod;
    }

    script_function::script_function(type_manager* mgr, script_type* tp, bind::wrapped_function* wrapped, bool is_ctor, bool is_dtor, script_module* mod) {
        m_ctx = mgr->m_ctx;

        name = wrapped->name;
        is_host = true;
        is_static = wrapped->is_static_method;
        is_method_of = tp;
        is_thiscall = tp && !wrapped->is_static_method;
        is_subtype_obj_ctor = tp && tp->requires_subtype && is_ctor;
        type = mgr->get(function_signature(mgr->ctx(), tp, wrapped, is_ctor));
        access.wrapped = wrapped;

        owner = mod ? mod : mgr->m_ctx->global();
        owner->add(this);
    }
};