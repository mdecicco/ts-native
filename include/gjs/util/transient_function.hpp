namespace gjs {
    //
    // TransientFunction
    //
    template <typename R, typename ...Args>
    template <typename S>
    R TransientFunction<R(Args...)>::Dispatch(void* target, Args... args) {
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
    R TransientFunction<R(Args...)>::operator()(void*, Args... args) const {
        return ((Dispatcher)m_Dispatcher)(m_Target, args...);
    }
};
