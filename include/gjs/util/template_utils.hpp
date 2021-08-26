#pragma once
#include <vector>
#include <string>

namespace gjs {
    class script_function;
    class script_type;
    class type_manager;
    class script_context;

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

        static void arg_types(type_manager* tpm, script_type** out, bool do_throw) {
            script_type* argTps[ArgCount] = { tpm->get<Args>(do_throw)... };
            for (u8 i = 0;i < ArgCount;i++) {
                out[i] = argTps[i];
            }
        }
    };

    template <typename T>
    inline const char* base_type_name() {
        return typeid(remove_all<T>::type).name();
    }

    template <typename T>
    void* to_arg(T& arg) {
        if constexpr (std::is_class_v<T>) {
            if constexpr (std::is_same_v<T, script_object>) {
                return arg.self();
            } else {
                // Pass non-pointer struct/class values as pointers to those values
                // The only way that functions accepting these types could be bound
                // is if the argument type is a pointer or a reference (which is
                // basically a pointer)
                return (void*)&arg;
            }
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