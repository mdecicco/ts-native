#pragma once
#include <gjs/common/script_function.h>

namespace gjs {
    template <class Cls>
    std::enable_if_t<std::is_class_v<Cls>, ffi::wrap_class<Cls>&>
    bind(script_context* ctx, const std::string& name) {
        return bind<Cls>(ctx->global(), name);
    }

    template <class Cls>
    std::enable_if_t<std::is_class_v<Cls>, ffi::wrap_class<Cls>&>
    bind(script_module* mod, const std::string& name) {
        // as long as wrap_class::finalize is called, this will be deleted when it should be
        ffi::wrap_class<Cls>* out = new ffi::wrap_class<Cls>(mod->types(), name);
        out->type->owner = mod;
        return *out;
    }

    template <class prim>
    std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, ffi::pseudo_class<prim>&>
    bind(script_context* ctx, const std::string& name) {
        return bind<prim>(ctx->global(), name);
    }

    template <class prim>
    std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, ffi::pseudo_class<prim>&>
    bind(script_module* mod, const std::string& name) {
        // as long as pseudo_class::finalize is called, this will be deleted when it should be
        ffi::pseudo_class<prim>* out = new ffi::pseudo_class<prim>(mod->types(), name);
        out->type->owner = mod;
        out->type->is_unsigned = std::is_unsigned_v<prim>;
        out->type->is_builtin = true;
        out->type->is_floating_point = std::is_floating_point_v<prim>;
        return *out;
    }

    template <typename Ret, typename... Args>
    script_function* bind(script_context* ctx, Ret(*func)(Args...), const std::string& name) {
        return bind<Ret, Args...>(ctx->global(), func, name);
    }

    template <typename Ret, typename... Args>
    script_function* bind(script_module* mod, Ret(*func)(Args...), const std::string& name) {
        ffi::wrapped_function* w = ffi::wrap(mod->context()->types(), name, func);
        return new script_function(mod->types(), nullptr, w, false, false, mod);
    }
};