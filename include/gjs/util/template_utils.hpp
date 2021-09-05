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
    class function_pointer;

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

    template<class T> struct remove_all { typedef T type; };
    template<class T> struct remove_all<T*> : remove_all<T> {};
    template<class T> struct remove_all<T&> : remove_all<T> {};
    template<class T> struct remove_all<T&&> : remove_all<T> {};
    template<class T> struct remove_all<T const> : remove_all<T> {};
    template<class T> struct remove_all<T volatile> : remove_all<T> {};
    template<class T> struct remove_all<T const volatile> : remove_all<T> {};
    template<class T> struct remove_all<T[]> : remove_all<T> {};

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

        static void arg_types(type_manager* tpm, script_type** out, bool do_throw);
    };

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

    template <typename L>
    std::enable_if_t<is_callable_object_v<std::remove_reference_t<L>>, script_type*>
    callback_signature(script_context* ctx, L&& l);

    template <typename T>
    inline const char* base_type_name();

    template <typename T>
    void* to_arg(T& arg);

    template <typename T>
    script_type* arg_type(script_context* ctx, T& arg);
    
    template <typename Ret, typename ...Args>
    script_function* function_search(script_context* ctx, const std::string& name, const std::vector<script_function*>& source);
};

#include <gjs/util/template_utils.inl>