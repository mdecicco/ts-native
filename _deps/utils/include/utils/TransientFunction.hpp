// https://gist.github.com/twoscomplement/030818a6c38c5a983482dc3a385a3ab8

// TransientFuction: A light-weight alternative to std::function [C++11]
// Pass any callback - including capturing lambdas - cheaply and quickly as a 
// function argument
// 
// Based on: 
// https://deplinenoise.wordpress.com/2014/02/23/using-c11-capturing-lambdas-w-vanilla-c-api-functions/
//
//  - No instantiation of called function at each call site
//  - Simple to use - use TransientFunction<> as the function argument
//  - Low cost: cheap setup, one indirect function call to invoke
//  - No risk of dynamic allocation (unlike std::function)
//
//  - Not C-friendly, unlike the original Reverse - but easy to adapt
//  - NOT PERSISTENT: Use for synchronous calls only
//  - No thought has been given to argument forwarding in this implementation 
//
// Compiles as C++11 using recent g++, clang, and msvc compilers
// Try it at https://godbolt.org
//
// Prior art:
// A proposal to the C++ ISO committee has been written by Vittorio Romeo,
// which lists a number of other instances of this same pattern:
// function_ref: a non-owning reference to a Callable
// https://vittorioromeo.info/Misc/p0792r1.html
// https://www.youtube.com/watch?v=6EQRoqELeWc
// https://vittorioromeo.info/index/blog/passing_functions_to_functions.html
//

#pragma once

// For std::decay
#include <type_traits>

namespace utils {
    template<typename>
    struct TransientFunction; // intentionally not defined

    template <typename R, typename ...Args>
    struct TransientFunction<R(Args...)> {
        using Dispatcher = R(*)(void*, Args...);
        using TargetFunctionRef = R(Args...);
    
        /**
         * @brief A pointer to the static function that will call the
         *        wrapped invokable object
         */
        Dispatcher m_Dispatcher;

        /**
         * @brief A pointer to the invokable object
         * 
         */
        void* m_Target;

        /**
         * @brief Dispatch() is instantiated by the TransientFunction
         *        constructor, which will store a pointer to the function
         *        in m_Dispatcher.
         * 
         * @tparam S The function type of the invokable object
         * 
         * @param target Pointer to the invokable object
         * @param args Arguments to invoke the invokable object with
         * 
         * @return Returns whatever the invokable object returns
         */
        template <typename S>
        static R Dispatch(void* target, Args... args) {
            return (*(S*)target)(args...);
        }
        
        /**
         * @brief Construct a new TransientFunction from some invokable object
         * 
         * @tparam T The type of the invokable object
         * 
         * @param target The invokable object.
         */
        template<typename T>
        TransientFunction(T&& target) : m_Dispatcher(&Dispatch<typename std::decay<T>::type>), m_Target(&target)
        { }

        /**
         * @brief Specialize for reference-to-function, to ensure that a valid
         *        pointer is stored.
         * 
         * @param target 
         */
        TransientFunction(TargetFunctionRef target) : m_Dispatcher(Dispatch<TargetFunctionRef>) {
            static_assert(
                sizeof(void*) == sizeof target, 
                "It will not be possible to pass functions by reference on this platform. "
                "Please use explicit function pointers i.e. foo(target) -> foo(&target)"
            );
            m_Target = (void*)target;
        }

        /**
         * @brief Calls the invokable object with some arguments
         * 
         * @param args The arguments to invoke the invokable object with, if any
         * @return Returns whatever the invokable object returns
         */
        R operator()(Args... args) const {
            return m_Dispatcher(m_Target, args...);
        }
    };
};