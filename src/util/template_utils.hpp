#pragma once
#include <vector>
#include <string>

namespace gjs {
    class script_function;
    class script_type;
    class script_context;

    template<class T> struct remove_all { typedef T type; };
    template<class T> struct remove_all<T*> : remove_all<T> {};
    template<class T> struct remove_all<T&> : remove_all<T> {};
    template<class T> struct remove_all<T&&> : remove_all<T> {};
    template<class T> struct remove_all<T const> : remove_all<T> {};
    template<class T> struct remove_all<T volatile> : remove_all<T> {};
    template<class T> struct remove_all<T const volatile> : remove_all<T> {};
    template<class T> struct remove_all<T[]> : remove_all<T> {};

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
    
    struct func_signature {
        func_signature() {};
        func_signature(const std::string& name, script_type* ret, const std::vector<script_type*>& arg_types);

        std::string to_string() const;
        
        std::string name;
        script_type* return_type;
        std::vector<script_type*> arg_types;
    };

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
    func_signature function_signature(script_context* ctx, const std::string& name) {
        func_signature s;
        s.name = name;
        s.return_type = ctx->global()->types()->get<Ret>();

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            s.arg_types = { ctx->global()->types()->get<Args>()... };
        }

        return s;
    }

    template <typename Ret, typename ...Args>
    func_signature function_signature(script_context* ctx, const std::string& name, Ret* ret, Args... args) {
        func_signature s;
        s.name = name;
        if constexpr (std::is_same_v<Ret, void>) s.return_type = ctx->global()->types()->get<void>();
        else s.return_type = arg_type(ctx, ret);

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            s.arg_types = { arg_type(ctx, args)... };
        }

        return s;
    }
};