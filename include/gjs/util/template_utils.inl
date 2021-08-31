#pragma once
namespace gjs {
    //
    // TransientFunction
    //
    template <typename R, typename ...Args>
    template <typename S>
    static R TransientFunction<R(Args...)>::Dispatch(void* target, Args... args) {
        return (*(S*)target)(args...);
    }

    template <typename R, typename ...Args>
    template <typename T>
    TransientFunction<R(Args...)>::TransientFunction(T&& target) : TransientFunctionBase({ &Dispatch<typename std::decay<T>::type>, nullptr }) {
        // Don't ask me why, but this line absolutely refuses to compile,
        // But for some reason, initializing it with : TransientFunctionBase({ &Dispatch<typename std::decay<T>::type>, nullptr })
        // is just fine.
        // m_Dispatcher = &Dispatch<typename std::decay<T>::type>;
        m_Target = &target;
    }

    template <typename R, typename ...Args>
    TransientFunction<R(Args...)>::TransientFunction(TargetFunctionRef target) : TransientFunctionBase({ Dispatch<TargetFunctionRef>, nullptr }) {
        static_assert(
            sizeof(void*) == sizeof target, 
            "It will not be possible to pass functions by reference on this platform. Please use explicit function pointers i.e. foo(target) -> foo(&target)"
        );
        // Don't ask me why, but this line absolutely refuses to compile,
        // But for some reason, initializing it with : TransientFunctionBase({ Dispatch<TargetFunctionRef>, nullptr })
        // is just fine.
        // m_Dispatcher = &Dispatch<TargetFunctionRef>;
        m_Target = (void*)target;
    }

    template <typename R, typename ...Args>
    TransientFunction<R(Args...)>::TransientFunction() {
        m_Dispatcher = nullptr;
        m_Target = nullptr;
    }

    template <typename R, typename ...Args>
    R TransientFunction<R(Args...)>::operator()(Args... args) const {
        return ((Dispatcher)m_Dispatcher)(m_Target, args...);
    }

    template <typename R, typename... Args>
    void FunctionTraits<R(*)(Args...)>::arg_types(type_manager* tpm, script_type** out, bool do_throw) {
        script_type* argTps[ArgCount] = { tpm->get<Args>(do_throw)... };
        for (u8 i = 0;i < ArgCount;i++) {
            out[i] = argTps[i];
        }
    }

    
    template <typename Ret, typename ...Args>
    void* raw_callback::make(Ret(*f)(Args...)) {
        using Fn = TransientFunction<Ret(Args...)>;
        function_pointer* fptr = new function_pointer(
            new script_function(
                script_context::current()->types(),
                script_context::current()->type<Ret(*)(Args...)>(),
                bind::wrap<Ret, Fn, Args...>(
                    script_context::current()->types(),
                    "operator ()",
                    &Fn::operator(),
                    true
                )
            )
        );
        raw_callback** cb = (raw_callback**)raw_callback::make(fptr);
        (*cb)->hostFn = {
            Fn::Dispatch<Fn::TargetFunctionRef>,
            (void*)f
        };
        (*cb)->ptr->bind_this(&(*cb)->hostFn);
        (*cb)->owns_func = true;
        (*cb)->owns_ptr = true;
        return (void*)cb;
    }

    template <typename L, typename Ret, typename ...Args>
    void* raw_callback::make(Ret (L::*f)(Args...) const) {
        using Fn = TransientFunction<Ret(Args...)>;
        function_pointer* fptr = new function_pointer(
            new script_function(
                script_context::current()->types(),
                script_context::current()->type<Ret(*)(Args...)>(),
                bind::wrap<Ret, Fn, Args...>(
                    script_context::current()->types(),
                    "operator ()",
                    &Fn::operator(),
                    true
                )
            )
        );
        raw_callback** cb = (raw_callback**)raw_callback::make(fptr);
        (*cb)->hostFn = {
            &Fn::Dispatch<typename std::decay<L>::type>,
            nullptr
        };
        (*cb)->ptr->bind_this(&(*cb)->hostFn);
        (*cb)->owns_func = true;
        (*cb)->owns_ptr = true;
        return (void*)cb;
    }

    template <typename L>
    std::enable_if_t<!std::is_function_v<std::remove_pointer_t<L>>, void*>
    raw_callback::make(L&& l) {
        static_assert(is_callable_object<std::remove_reference_t<L>>::value, "Object type L passed to raw_callback::make is not a lambda");
        raw_callback** cb = (raw_callback**)raw_callback::make(&std::decay_t<decltype(l)>::operator());
        (*cb)->hostFn.m_Target = (void*)&l;
        return (void*)cb;
    }


    //
    // callback
    //
    template <typename Ret, typename ...Args>
    script_type* callback<Ret(*)(Args...)>::type(script_context* ctx) {
        return cdecl_signature<Ret, Args...>(ctx);
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::callback(callback<Ret(*)(Args...)>& rhs) {
        data = rhs.data;
        rhs.data = nullptr;
        if (data && data->free_self) {
            data->free_self = false;

            // see comment in ~callback()
            delete (void*)&rhs;
        }
    }

    template <typename Ret, typename ...Args>
    template <typename F>
    callback<Ret(*)(Args...)>::callback(std::enable_if_t<std::is_invocable_r_v<Ret, F, Args...>, F>&& f) {
        data = new raw_callback({
            false,
            true,
            true,
            (TransientFunctionBase)TransientFunction<Ret(Args...)>(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    script_context::current()->type<Ret(*)(Args...)>(),
                    bind::wrap<Ret, Fn, Args...>(
                        script_context::current()->types(),
                        "operator ()",
                        &Fn::operator(),
                        true
                    )
                )
            )
        });
        data->ptr->bind_this(&data->hostFn);
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::callback(callback<Ret(*)(Args...)>::TargetFunctionRef f) {
        data = new raw_callback({
            false,
            true,
            true,
            TransientFunction<Ret(Args...)>(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    script_context::current()->type<Ret(*)(Args...)>(),
                    bind::wrap<Ret, Fn, Args...>(
                        script_context::current()->types(),
                        "operator ()",
                        &Fn::operator(),
                        true
                    )
                )
            )
        });
        data->ptr->bind_this(&data->hostFn);
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::callback(Fn f) {
        data = new raw_callback({
            false,
            true,
            true,
            (TransientFunctionBase)std::move(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    script_context::current()->type<Ret(*)(Args...)>(),
                    bind::wrap<Ret, Fn, Args...>(
                        script_context::current()->types(),
                        "operator ()",
                        &Fn::operator(),
                        true
                    )
                )
            )
        });
        data->ptr->bind_this(&data->hostFn);
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::callback(function_pointer* fptr) {
        data = new raw_callback({
            false,
            false,
            false,
            { nullptr, nullptr },
            fptr
        });
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::~callback() {
        if (!data) return;
        if (data->owns_func && data->ptr) {
            delete data->ptr->target->access.wrapped;
            delete data->ptr->target;
        }

        if (data->owns_ptr && data->ptr) {
            delete data->ptr;
        }

        raw_callback* dptr = data;
        data = nullptr;
        if (dptr->free_self) {
            // hear me out...
            // If free_self is true, then this object was created via raw_callback::make(funcId, data, dataSz)
            // That function allocates a raw_callback, then allocates a void** and stores the callback in it
            // That allocated void** IS 'this' here.
            //
            // This should only happen when the callback is passed from a script to the host, because the script
            // can't instantiate the callback<T> to pass to a function, it must generate an equivalent data structure
            // and pass that as the parameter instead.
            // Essentially, this is happening:
            // void fun(callback<T> param) {...}
            // raw_callback* cb = new raw_callback({ cb_data... }); // this->data
            // raw_callback** cbp = new raw_callback*(cb);          // this
            // fun(cbp); // ignore the cast from a pointer type to a non-pointer type, FFIs are black magic
            delete (void*)this;
        }

        delete dptr;
    }

    template <typename Ret, typename ...Args>
    Ret callback<Ret(*)(Args...)>::operator() (Args ... args) {
        if (!data) {
            throw error::runtime_exception(error::ecode::r_callback_missing_data);
        }

        if (data->ptr->data) {
            if constexpr (std::is_same_v<Ret, void>) data->ptr->target->call(data->ptr->self_obj(), data->ptr->data->data(), args...);
            else return data->ptr->target->call(data->ptr->self_obj(), data->ptr->data->data(), args...);
        } else {
            if constexpr (std::is_same_v<Ret, void>) data->ptr->target->call(data->ptr->self_obj(), args...);
            else return data->ptr->target->call(data->ptr->self_obj(), args...);
        }
    }

    //
    // Misc
    //
    template <typename Ret, typename... Args>
    script_type* cdecl_signature(script_context* ctx) {
        script_type* rt = ctx->type<Ret>();
        if (!rt) throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_return, base_type_name<Ret>());
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { ctx->type<Args>()... };

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
            nullptr
        ));
    }

    template <typename Cls, typename Ret, typename... Args>
    script_type* thiscall_signature(script_context* ctx) {
        script_type* rt = ctx->type<Ret>();
        if (!rt) throw error::bind_exception(error::ecode::b_cannot_get_signature_unbound_return, base_type_name<Ret>());
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { ctx->type<Args>()... };

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
            ctx->type<Cls>()
        ));
    }

    template <typename Ret, typename... Args>
    script_type* signature(script_context* ctx, Ret (*f)(Args...)) {
        return cdecl_signature<Ret, Args...>(ctx);
    }

    template <typename L, typename Ret, typename ...Args>
    script_type* signature(script_context* ctx, Ret (L::*f)(Args...) const) {
        return cdecl_signature<Ret, Args...>(ctx);
    }

    template <typename L, typename Ret, typename ...Args>
    script_type* signature(script_context* ctx, Ret (L::*f)(Args...)) {
        return cdecl_signature<Ret, Args...>(ctx);
    }

    template <typename L>
    std::enable_if_t<is_callable_object_v<std::remove_reference_t<L>>, script_type*>
    signature(script_context* ctx, L&& l) {
        static_assert(is_callable_object<std::remove_reference_t<L>>::value, "Object type L passed to raw_callback::make is not a lambda");
        return signature(ctx, &std::decay_t<decltype(l)>::operator());
    }

    template <typename T>
    inline const char* base_type_name() {
        return typeid(remove_all<T>::type).name();
    }

    template <typename T>
    void* to_arg(T& arg) {
        if constexpr (std::is_class_v<T>) {
            if constexpr (std::is_same_v<T, script_object>) {
                return arg.self();
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
            if constexpr (std::is_same_v<remove_all<T>::type, script_object>) {
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
        if constexpr (std::is_same_v<remove_all<T>::type, script_object>) {
            if constexpr (std::is_pointer_v<T>) return arg->type();
            else return arg.type();
        } else if constexpr(std::is_function_v<std::remove_pointer_t<T>>) {
            return signature(ctx, arg);
        } else if constexpr(is_callable_object_v<std::remove_reference_t<T>>) {
            return signature(ctx, arg);
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
};