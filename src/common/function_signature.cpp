#include <gjs/common/function_signature.h>
#include <gjs/vm/register.h>

#include <gjs/gjs.hpp>

namespace gjs {
    function_signature::function_signature(script_context* ctx, script_type* tp, ffi::wrapped_function* wrapped, bool is_ctor) {
        method_of = tp;
        return_type = wrapped->return_type;
        return_loc = return_type->size == 0 ? vm_register::register_count : vm_register::v0;
        if (return_type->is_floating_point) return_loc = vm_register::vf0;
        returns_pointer = false;
        returns_on_stack = false;
        is_thiscall = false;
        explicit_argc = 0;
        implicit_argc = 0;
        returns_on_stack = !wrapped->ret_is_ptr && !return_type->is_primitive && return_type->size != 0;
        returns_pointer = wrapped->ret_is_ptr;
        is_thiscall = wrapped->pass_this || (tp && !wrapped->is_static_method);
        bool is_subtype_obj_ctor = tp && tp->requires_subtype && is_ctor;

        u16 gp_arg = (u16)vm_register::a0;
        u16 fp_arg = (u16)vm_register::fa0;
        if (is_thiscall) args.push_back({ tp, vm_register(gp_arg++), argument::implicit_type::this_ptr, true });
        if (is_subtype_obj_ctor) args.push_back({ type_of<u64>(ctx), vm_register(gp_arg++), argument::implicit_type::moduletype_id, false });
        if (returns_on_stack) args.push_back({ type_of<void*>(ctx), vm_register(gp_arg++), argument::implicit_type::ret_addr, true });
        implicit_argc = (u8)args.size();

        // args
        for (u8 i = 0;i < wrapped->arg_types.size();i++) {
            script_type* atp = wrapped->arg_types[i];

            if (atp->is_floating_point) args.push_back({ atp, vm_register(fp_arg++), argument::implicit_type::not_implicit, wrapped->arg_is_ptr[i] });
            else args.push_back({ atp, vm_register(gp_arg++), argument::implicit_type::not_implicit, wrapped->arg_is_ptr[i] });
            explicit_argc++;
        }
    }

    function_signature::function_signature(script_context* ctx, script_type* ret, bool ret_ptr, script_type** argtps, u8 argc, script_type* _method_of, bool is_ctor, bool is_static_method, bool is_callback) {
        method_of = _method_of;
        return_type = ret;
        return_loc = return_type->size == 0 ? vm_register::register_count : vm_register::v0;
        if (return_type->is_floating_point) return_loc = vm_register::vf0;
        returns_pointer = ret_ptr;
        returns_on_stack = !ret->is_primitive && !ret_ptr && ret->size != 0;
        explicit_argc = 0;
        implicit_argc = 0;
        is_thiscall = method_of && !is_static_method;
        bool is_subtype_obj_ctor = method_of && method_of->requires_subtype && is_ctor;

        u16 gp_arg = (u16)vm_register::a0;
        u16 fp_arg = (u16)vm_register::fa0;
        if (is_callback) args.push_back({ type_of<void*>(ctx), vm_register(gp_arg++), argument::implicit_type::capture_data_ptr, true });
        if (is_thiscall) args.push_back({ method_of, vm_register(gp_arg++), argument::implicit_type::this_ptr, true });
        if (is_subtype_obj_ctor) args.push_back({ type_of<u64>(ctx), vm_register(gp_arg++), argument::implicit_type::moduletype_id, false });
        if (returns_on_stack) args.push_back({ type_of<void*>(ctx), vm_register(gp_arg++), argument::implicit_type::ret_addr, true });
        implicit_argc = (u8)args.size();

        // args
        for (u8 i = 0;i < argc;i++) {
            if (argtps[i]->is_floating_point) args.push_back({ argtps[i], vm_register(fp_arg++), argument::implicit_type::not_implicit, false });
            else args.push_back({ argtps[i], vm_register(gp_arg++), argument::implicit_type::not_implicit, false });
            explicit_argc++;
        }
    }

    function_signature::function_signature() {
        return_type = nullptr;
        return_loc = vm_register::register_count;
        returns_pointer = false;
        returns_on_stack = false;
        is_thiscall = false;
        explicit_argc = 0;
        implicit_argc = 0;
    }

    const function_signature::argument& function_signature::explicit_arg(u8 idx) const {
        return args[idx + (args.size() - explicit_argc)];
    }

    std::string function_signature::to_string() const {
        std::string out = "(";

        for (u8 i = 0;i < args.size();i++) {
            if (i > 0) out += ",";
            if (args[i].implicit != argument::implicit_type::not_implicit) out += "$";
            out += args[i].tp->name;
        }

        out += ")";

        if (return_type) return return_type->name + out;
        return out;
    }
    std::string function_signature::to_string(const std::string& funcName, script_type* method_of, script_module* mod) const {
        std::string out;
        if (mod) out += mod->name() + "::";
        //if (method_of) out += method_of->name + "::";
        out += funcName + "(";

        for (u8 i = 0;i < args.size();i++) {
            if (i > 0) out += ",";
            if (args[i].implicit != argument::implicit_type::not_implicit) out += "$";
            out += args[i].tp->name;
        }

        out += ")";

        if (return_type) return return_type->name + " " + out;
        return out;
    }
};
