#include <vm_function.h>
#include <vm_type.h>
#include <bind.h>
#include <util.h>
#include <register.h>
#include <context.h>

namespace gjs {
    vm_function::vm_function(vm_context* ctx, const std::string _name, address addr) {
        m_ctx = ctx;
        name = _name;
        access.entry = addr;
        is_host = false;
        signature.returns_on_stack = false;
        signature.is_subtype_obj_ctor = false;
        is_method_of = nullptr;
        is_static = false;
    }

    vm_function::vm_function(type_manager* mgr, vm_type* tp, bind::wrapped_function* wrapped, bool is_ctor, bool is_dtor) {
        is_method_of = tp;
        m_ctx = mgr->m_ctx;
        signature.returns_on_stack = false;
        signature.return_loc = vm_register::v0;
        signature.returns_pointer = wrapped->ret_is_ptr;
        name = wrapped->name;
        signature.text = wrapped->sig;
        signature.is_subtype_obj_ctor = tp && tp->requires_subtype && is_ctor;
        is_static = false;
        for (u8 i = 0;i < wrapped->arg_types.size();i++) {
            vm_type* atp = nullptr;
            if (std::string(wrapped->arg_types[i].name()) == "void" && wrapped->ret_is_ptr) {
                atp = mgr->get("void*"); // some object or primitive pointer
            } else if (tp && i == 1 && tp->requires_subtype && is_ctor) {
                atp = mgr->get("void*"); // vm_type*
            } else if (tp && i == 0 && tp->requires_subtype && !is_ctor && !is_dtor) {
                atp = mgr->get("void*"); // vm_type*
            } else atp = mgr->get(wrapped->arg_types[i].name());

            if (!atp) {
                throw bind_exception(format("Arg '%d' of function '%s' is of type '%s' that has not been bound yet", i + 1, name.c_str(), wrapped->arg_types[i].name()));
            }
            signature.arg_types.push_back(atp);

            vm_register last_a = vm_register(integer(vm_register::a0) - 1);
            vm_register last_f = vm_register(integer(vm_register::f0) - 1);

            for (u8 a = 0;a < signature.arg_locs.size();a++) {
                vm_register l = signature.arg_locs[a];
                if (l >= vm_register::a0 && l <= vm_register::a7) last_a = l;
                else last_f = l;
            }

            if (std::string(wrapped->arg_types[i].name()) == "float") signature.arg_locs.push_back(vm_register(integer(last_f) + 1));
            else signature.arg_locs.push_back(vm_register(integer(last_a) + 1));
        }

        if (std::string(wrapped->return_type.name()) == "void" && wrapped->ret_is_ptr) {
            signature.return_type = mgr->get("void*");
        } else signature.return_type = mgr->get(wrapped->return_type.name());

        if (!signature.return_type) {
            throw bind_exception(format("Return value of function '%s' is of type '%s' that has not been bound yet", name.c_str(), wrapped->return_type.name()));
        }

        signature.is_thiscall = tp && !wrapped->is_static_method;
        access.wrapped = wrapped;
        mgr->m_ctx->add(this);
    }

    void vm_function::arg(vm_type* type) {
        if (is_host) throw bind_exception("Cannot specify arguments for host functions");
        if (!type) throw bind_exception("No type specified for argument");
        signature.arg_types.push_back(type);
        vm_register last_a = vm_register(integer(vm_register::a0) - 1);
        vm_register last_f = vm_register(integer(vm_register::f0) - 1);

        for (u8 a = 0;a < signature.arg_locs.size();a++) {
            vm_register l = signature.arg_locs[a];
            if (l >= vm_register::a0 && l <= vm_register::a7) last_a = l;
            else last_f = l;
        }

        if (type->name == "decimal") signature.arg_locs.push_back(vm_register(integer(last_f) + 1));
        else signature.arg_locs.push_back(vm_register(integer(last_a) + 1));
    }
};