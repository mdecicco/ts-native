#pragma once

namespace gjs {
    struct TransientFunctionBase {
        // A pointer to the static function that will call the wrapped invokable object
        void* m_Dispatcher;
        // A pointer to the invokable object
        void* m_Target;
    };

    // Thank you to Jonathan Adamczewski for the TransientFunction code
    // https://brnz.org/hbr/?p=1767
    template<typename>
    struct TransientFunction; // intentionally not defined

    template<typename R, typename ...Args>
    struct TransientFunction<R(Args...)> : public TransientFunctionBase {
        using PtrType = R(*)(Args...);
        using Dispatcher = R(*)(void*, Args...);

        template<typename S>
        static R Dispatch(void* target, Args... args);

        template<typename T>
        TransientFunction(T&& target);

        // Specialize for reference-to-function, to ensure that a valid pointer is
        // stored.
        using TargetFunctionRef = R(Args...);
        TransientFunction(TargetFunctionRef target);

        TransientFunction();

        R operator()(void*, Args... args) const;
    };
};
