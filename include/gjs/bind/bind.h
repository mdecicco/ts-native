#pragma once
#include <gjs/bind/ffi.h>

// If a function is overloaded, this can help select it to pass to a binding function
// Usage:
//    ctx->bind(FUNC_PTR(some_func, return_type, arg_type0, arg_type1, ... arg_typeN), "some_func");
#define FUNC_PTR(func, ret, ...) ((ret(*)(__VA_ARGS__))func)

// If a class method is overloaded, this can help select it to pass to a binding function
// Usage:
//    ctx->bind(FUNC_PTR(some_class, some_func, return_type, arg_type0, arg_type1, ... arg_typeN), "some_func");
#define METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__))&cls::method)

// If a const class method is overloaded, this can help select it to pass to a binding function
// Usage:
//    ctx->bind(FUNC_PTR(some_class, some_func, return_type, arg_type0, arg_type1, ... arg_typeN), "some_func");
#define CONST_METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__) const)&cls::method)

namespace gjs {
    // Bind class to global module of current context
    template <class Cls>
    std::enable_if_t<std::is_class_v<Cls>, ffi::wrap_class<Cls>&>
    bind(script_context* ctx, const std::string& name);

    // Bind class to specific module
    template <class Cls>
    std::enable_if_t<std::is_class_v<Cls>, ffi::wrap_class<Cls>&>
    bind(script_module* mod, const std::string& name);

    // Bind primitive type to global module of current context
    template <class prim>
    std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, ffi::pseudo_class<prim>&>
    bind(script_context* ctx, const std::string& name);

    // Bind primitive type to specific module
    template <class prim>
    std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, ffi::pseudo_class<prim>&>
    bind(script_module* mod, const std::string& name);

    // Bind cdecl function to global module of current context
    template <typename Ret, typename... Args>
    script_function* bind(script_context* ctx, Ret(*func)(Args...), const std::string& name);

    // Bind cdecl function to specific module
    template <typename Ret, typename... Args>
    script_function* bind(script_module* mod, Ret(*func)(Args...), const std::string& name);
};

#include <gjs/bind/bind.inl>