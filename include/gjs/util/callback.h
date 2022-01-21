#pragma once
#include <gjs/common/script_context.h>
#include <gjs/common/function_pointer.h>
#include <gjs/util/transient_function.h>

namespace gjs {
    struct raw_callback {
        bool free_self;
        bool owns_ptr;
        bool owns_func;
        TransientFunctionBase hostFn;
        function_pointer* ptr;

        static void* make(function_pointer* fptr);
        static void* make(u32 fid, void* data, u64 dataSz);

        template <typename Ret, typename ...Args>
        static void* make(Ret(*f)(Args...));

        template <typename L, typename Ret, typename ...Args>
        static void* make(Ret (L::*f)(Args...) const);

        static void destroy(raw_callback** cbp);

        template <typename L>
        static std::enable_if_t<!std::is_function_v<std::remove_pointer_t<L>>, void*>
        make(L&& l);
    };

    template <typename T>
    struct callback;

    template <typename Ret, typename ...Args>
    struct callback<Ret(*)(Args...)> : FunctionTraitsBase<Ret, Args...> {
        using Fn = TransientFunction<Ret(Args...)>;

        static script_type* type(script_context* ctx);

        callback(callback<Ret(*)(Args...)>& rhs);

        template <typename F>
        callback(std::enable_if_t<std::is_invocable_r_v<Ret, F, Args...>, F>&& f);

        using TargetFunctionRef = Ret(Args...);
        callback(TargetFunctionRef f);

        callback(Fn f);

        callback(function_pointer* fptr);

        ~callback();

        Ret operator() (Args ... args);

        raw_callback* data;
    };

    template <typename, typename = void> struct is_callback : std::false_type {};
    template <typename T> struct is_callback<T, std::void_t<decltype(T::data)>> : std::is_convertible<decltype(T::data), raw_callback*> {};
    template <typename T> constexpr bool is_callback_v = is_callback<T>::value;
};
