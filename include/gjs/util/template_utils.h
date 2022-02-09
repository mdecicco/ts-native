#pragma once
#include <vector>
#include <string>
#include <gjs/common/types.h>
#include <gjs/common/errors.h>

namespace gjs {
    class script_function;
    class script_type;
    class type_manager;
    class script_context;
    class script_module;
    class function_pointer;

    template <typename T>
    class is_callable_object {
        typedef char one;
        typedef long two;

        template <typename C> static one test( decltype(&C::operator()) ) ;
        template <typename C> static two test(...);
        public:
            enum { value = sizeof(test<T>(0)) == sizeof(char) };
    };

    template <typename T>
    constexpr bool is_callable_object_v = is_callable_object<T>::value;

    template <typename Ret, typename... Args>
    script_type* cdecl_signature(script_context* ctx, bool is_callback = false);

    template <typename Cls, typename Ret, typename... Args>
    script_type* thiscall_signature(script_context* ctx, bool is_callback = false);

    template <typename Ret, typename... Args>
    script_type* callback_signature(script_context* ctx, Ret (*f)(Args...));

    template <typename L, typename Ret, typename ...Args>
    script_type* callback_signature(script_context* ctx, Ret (L::*f)(Args...));

    template <typename L, typename Ret, typename ...Args>
    script_type* callback_signature(script_context* ctx, Ret (L::*f)(Args...));

    template <typename L>
    std::enable_if_t<is_callable_object_v<std::remove_reference_t<L>>, script_type*>
    callback_signature(script_context* ctx, L&& l);

    template<class T> struct remove_all { typedef T type; };
    template<class T> struct remove_all<T*> : remove_all<T> {};
    template<class T> struct remove_all<T&> : remove_all<T> {};
    template<class T> struct remove_all<T&&> : remove_all<T> {};
    template<class T> struct remove_all<T const> : remove_all<T> {};
    template<class T> struct remove_all<T volatile> : remove_all<T> {};
    template<class T> struct remove_all<T const volatile> : remove_all<T> {};
    template<class T> struct remove_all<T[]> : remove_all<T> {};

    template <typename R, typename... Args>
    struct FunctionTraitsBase {
        using RetType = R;
        using ArgTypes = std::tuple<Args...>;
        static constexpr std::size_t ArgCount = sizeof...(Args);
        template <std::size_t N>
        using NthArg = std::tuple_element_t<N, ArgTypes>;
    };

    template <typename F> struct FunctionTraits;

    template <typename R, typename... Args>
    struct FunctionTraits<R(*)(Args...)> : FunctionTraitsBase<R, Args...> {
        using Pointer = R(*)(Args...);
    };

    template <typename T>
    inline const char* base_type_name();

    template <typename T>
    void* to_arg(T& arg);

    template <typename T>
    script_type* arg_type(script_context* ctx, T& arg);

    template <typename Ret, typename ...Args>
    script_function* function_search(script_context* ctx, const std::string& name, const std::vector<script_function*>& source);

    template <typename Ret, typename ...Args>
    script_function* function_search(script_module* mod, const std::string& name);
};
