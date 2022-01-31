namespace gjs {
    template <typename Ret, typename ...Args>
    void* raw_callback::make(Ret(*f)(Args...)) {
        using Fn = TransientFunction<Ret(Args...)>;
        function_pointer* fptr = new function_pointer(
            new script_function(
                script_context::current()->types(),
                callback_signature<Ret, Args...>(script_context::current(), (Ret(*)(Args...))nullptr),
                ffi::wrap<Ret, Fn, void*, Args...>(
                    script_context::current()->types(),
                    "operator ()",
                    &Fn::operator(),
                    true
                )
            )
        );
        raw_callback** cb = (raw_callback**)raw_callback::make(fptr);
        (*cb)->hostFn = {
            Fn::template Dispatch<Fn::TargetFunctionRef>,
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
                callback_signature<Ret, Args...>(script_context::current(), (Ret(*)(Args...))nullptr),
                ffi::wrap<Ret, Fn, void*, Args...>(
                    script_context::current()->types(),
                    "operator ()",
                    &Fn::operator(),
                    true
                )
            )
        );
        raw_callback** cb = (raw_callback**)raw_callback::make(fptr);
        (*cb)->hostFn = {
            &Fn::template Dispatch<typename std::decay<L>::type>,
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
        return cdecl_signature<Ret, Args...>(ctx, true);
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::callback(callback<Ret(*)(Args...)>& rhs) {
        data = rhs.data;
        rhs.data = nullptr;
        if (data && data->free_self) {
            data->free_self = false;
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
            true,
            (TransientFunctionBase)TransientFunction<Ret(Args...)>(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    callback_signature<Ret, Args...>(script_context::current(), (Ret(*)(Args...))nullptr),
                    ffi::wrap<Ret, Fn, void*, Args...>(
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
            true,
            TransientFunction<Ret(Args...)>(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    callback_signature<Ret, Args...>(script_context::current(), (Ret(*)(Args...))nullptr),
                    ffi::wrap<Ret, Fn, void*, Args...>(
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
            true,
            (TransientFunctionBase)std::move(f),
            new function_pointer(
                new script_function(
                    script_context::current()->types(),
                    callback_signature<Ret, Args...>(script_context::current(), (Ret(*)(Args...))nullptr),
                    ffi::wrap<Ret, Fn, void*, Args...>(
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
            true,
            { nullptr, nullptr },
            fptr
        });
    }

    template <typename Ret, typename ...Args>
    callback<Ret(*)(Args...)>::~callback() {
        if (!data) return;
        raw_callback::destroy((raw_callback**)this);
    }

    template <typename Ret, typename ...Args>
    Ret callback<Ret(*)(Args...)>::operator() (Args ... args) {
        if (!data) {
            throw error::runtime_exception(error::ecode::r_callback_missing_data);
        }

        if constexpr (std::is_same_v<Ret, void>) call(data, args...);
        else return call(data, args...);
    }
};
