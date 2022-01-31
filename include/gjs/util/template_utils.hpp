#pragma once
namespace gjs {
    //
    // Misc
    //
    template <typename Ret, typename... Args>
    script_type* cdecl_signature(script_context* ctx, bool is_cb) {
        script_type* rt = type_of<Ret>(ctx);
        if (!rt) throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_return, base_type_name<Ret>());
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { type_of<Args>(ctx)... };

        for (u8 i = 0;i < argc;i++) {
            if (!argTps[i]) {
                const char* argtypenames[] = { base_type_name<Args>()... };
                throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_arg, argtypenames[i], i);
            }
        }

        return ctx->types()->get(function_signature(
            ctx,
            rt,
            (std::is_reference_v<Ret> || std::is_pointer_v<Ret>),
            argTps,
            argc,
            nullptr,
            false,
            false,
            is_cb
        ));
    }

    template <typename Cls, typename Ret, typename... Args>
    script_type* thiscall_signature(script_context* ctx, bool is_cb) {
        script_type* rt = type_of<Ret>(ctx);
        if (!rt) throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_return, base_type_name<Ret>());
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { type_of<Args>(ctx)... };

        for (u8 i = 0;i < argc;i++) {
            if (!argTps[i]) {
                const char* argtypenames[] = { base_type_name<Args>()... };
                throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_arg, argtypenames[i], i);
            }
        }

        return ctx->types()->get(function_signature(
            ctx,
            rt,
            (std::is_reference_v<Ret> || std::is_pointer_v<Ret>),
            argTps,
            argc,
            type_of<Cls>(ctx),
            false,
            false,
            is_cb
        ));
    }

    template <typename Ret, typename... Args>
    script_type* callback_signature(script_context* ctx, Ret (*f)(Args...)) {
        return cdecl_signature<Ret, Args...>(ctx, true);
    }

    template <typename L, typename Ret, typename ...Args>
    script_type* callback_signature(script_context* ctx, Ret (L::*f)(Args...) const) {
        return cdecl_signature<Ret, Args...>(ctx, true);
    }

    template <typename L, typename Ret, typename ...Args>
    script_type* callback_signature(script_context* ctx, Ret (L::*f)(Args...)) {
        return cdecl_signature<Ret, Args...>(ctx, true);
    }

    template <typename L>
    std::enable_if_t<is_callable_object_v<std::remove_reference_t<L>>, script_type*>
    callback_signature(script_context* ctx, L&& l) {
        static_assert(is_callable_object<std::remove_reference_t<L>>::value, "Object type L passed to raw_callback::make is not a lambda");
        return callback_signature(ctx, &std::decay_t<decltype(l)>::operator());
    }

    template <typename T>
    inline const char* base_type_name() {
        return typeid(typename remove_all<T>::type).name();
    }

    template <typename T>
    void* to_arg(T& arg) {
        if constexpr (std::is_class_v<T>) {
            if constexpr (std::is_same_v<T, script_object>) {
                return arg.self();
            } else if constexpr (is_callback_v<T>) {
                return (void*)&arg;
            } else if constexpr (is_callable_object_v<std::remove_reference_t<T>>) {
                return raw_callback::make(arg);
            } else {
                // Pass non-pointer struct/class values as pointers to those values
                // The only way that functions accepting these types could be bound
                // is if the argument type is a pointer or a reference (which is
                // basically a pointer)
                return (void*)&arg;
            }
        } else if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
            return raw_callback::make(arg);
        } else {
            if constexpr (std::is_same_v<typename remove_all<T>::type, script_object>) {
                if constexpr (std::is_pointer_v<T>) return arg->self();
                else return arg.self();
            } else {
                // Otherwise, cast the value to void*
                return *reinterpret_cast<void**>(&arg);
            }
        }
    }

    template <typename T>
    script_type* arg_type(script_context* ctx, T& arg) {
        if constexpr (std::is_same_v<typename remove_all<T>::type, script_object>) {
            if constexpr (std::is_pointer_v<T>) return arg->type();
            else return arg.type();
        } else if constexpr(std::is_function_v<std::remove_pointer_t<T>>) {
            return callback_signature(ctx, arg);
        } else if constexpr(is_callable_object_v<std::remove_reference_t<T>>) {
            return callback_signature(ctx, arg);
        } else {
            return ctx->global()->types()->get<T>();
        }
    }

    template <typename Ret, typename ...Args>
    script_function* function_search(script_context* ctx, const std::string& name, const std::vector<script_function*>& source) {
        if (source.size() == 0) return nullptr;

        script_type* ret = ctx->global()->types()->get<Ret>();
        if (!ret) return nullptr;

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { ctx->global()->types()->get<Args>()... };

            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) return nullptr;
            }
            return function_search(name, source, ret, { ctx->global()->types()->get<Args>()... });
        }

        return function_search(name, source, ret, { });
    }

    template <typename Ret, typename ...Args>
    script_function* function_search(script_module* mod, const std::string& name) {
        if (!mod->has_function(name)) return nullptr;
        auto funcs = mod->function_overloads(name);
        return function_search<Ret, Args...>(mod->context(), name, funcs);
    }
};
