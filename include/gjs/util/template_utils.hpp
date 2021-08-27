#pragma once
#include <vector>
#include <string>
#include <gjs/common/types.h>

namespace gjs {
    class script_function;
    class script_type;
    class type_manager;
    class script_context;
    class function_pointer;

    template <typename Ret, typename... Args>
    script_type* cdecl_signature(script_context* ctx) {
        script_type* rt = ctx->type<Ret>();
        if (!rt) throw bind_exception(format("Cannot construct signature type for function with unbound return type '%s'", base_type_name<Ret>()));
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { ctx->type<Args>()... };

        for (u8 i = 0;i < argc;i++) {
            if (!argTps[i]) {
                throw bind_exception(format("Cannot construct signature type for function with unbound argument type (arg index %d)", i));
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
        if (!rt) throw bind_exception(format("Cannot construct signature type for function with unbound return type '%s'", base_type_name<Ret>()));
        constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
        script_type* argTps[argc] = { ctx->type<Args>()... };

        for (u8 i = 0;i < argc;i++) {
            if (!argTps[i]) {
                throw bind_exception(format("Cannot construct signature type for function with unbound argument type (arg index %d)", i));
            }
        }

        return ctx->types()->get(function_signature(
            ctx,
            rt,
            (std::is_reference_v<Ret> || std::is_pointer_v<Ret>),
            argTps,
            argc,
            ctx->type<Cls>
        ));
    }


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
        static R Dispatch(void* target, Args... args) {
            return (*(S*)target)(args...);
        }

        template<typename T>
        TransientFunction(T&& target) : m_Dispatcher(&Dispatch<typename std::decay<T>::type>), m_Target(&target) { }

        // Specialize for reference-to-function, to ensure that a valid pointer is 
        // stored.
        using TargetFunctionRef = R(Args...);
        TransientFunction(TargetFunctionRef target) : m_Dispatcher(Dispatch<TargetFunctionRef>) {
            static_assert(
                sizeof(void*) == sizeof target, 
                "It will not be possible to pass functions by reference on this platform. "
                "Please use explicit function pointers i.e. foo(target) -> foo(&target)"
                );
            m_Target = (void*)target;
        }

        TransientFunction() : m_Dispatcher(nullptr), m_Target(nullptr) { }

        R operator()(Args... args) const {
            return *((Dispatcher)m_Dispatcher)(m_Target, args...);
        }
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

        static void arg_types(type_manager* tpm, script_type** out, bool do_throw) {
            script_type* argTps[ArgCount] = { tpm->get<Args>(do_throw)... };
            for (u8 i = 0;i < ArgCount;i++) {
                out[i] = argTps[i];
            }
        }
    };

    struct raw_callback {
        bool free_self;
        bool owns_ptr;
        bool owns_func;
        TransientFunctionBase hostFn;
        function_pointer* ptr;

        static raw_callback* make(function_pointer* fptr);
        static void* make(u32 fid, void* data, u64 dataSz);
    };

    template <typename T>
    struct callback;

    template <typename Ret, typename ...Args>
    struct callback<Ret(*)(Args...)> : FunctionTraitsBase<Ret, Args...> {
        using Fn = TransientFunction<Ret(Args...)>;

        static script_type* type(script_context* ctx) {
            return cdecl_signature<Ret, Args...>(ctx);
        }

        callback(callback<Ret(*)(Args...)>& rhs) {
            data = rhs.data;
            rhs.data = nullptr;
        }

        callback(Fn f) {
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
                        );
                    )
                );
            });
            data->ptr->bind_this(&data->hostFn);
        }

        template <typename Obj, std::enable_if_t<std::is_invocable_r_v<Ret, Obj, Args...>>>
        callback(Obj o) {
            data = new raw_callback({
                false,
                true,
                true,
                (TransientFunctionBase)TransientFunction<Ret(Args...)>(o),
                new function_pointer(
                    new script_function(
                        script_context::current()->types(),
                        script_context::current()->type<Ret(*)(Args...)>(),
                        bind::wrap<Ret, Fn, Args...>(
                            script_context::current()->types(),
                            "operator ()",
                            &Fn::operator(),
                            true
                        );
                    )
                );
            });
            data->ptr->bind_this(&data->hostFn);
        }

        callback(function_pointer* fptr) {
            data = new raw_callback({
                false,
                false,
                false,
                { nullptr, nullptr },
                fptr
            });
        }

        ~callback() {
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

        Ret operator() (Args ... args) {
            if (!data) {
                // throw runtime exception
            }

            if (data->ptr->data) {
                if constexpr (std::is_same_v<Ret, void>) data->ptr->target->call(data->ptr->self_obj(), data->ptr->data->data(), args...);
                else return data->ptr->target->call(scriptFunc->self_obj(), scriptFunc->data->data(), args...);
            } else {
                if constexpr (std::is_same_v<Ret, void>) data->ptr->target->call(data->ptr->self_obj(), args...);
                else return data->ptr->target->call(data->ptr->self_obj(), args...);
            }
        }

        raw_callback* data;
    };
    
    template <typename, typename = void> struct is_callback : std::false_type {};
    template <typename T> struct is_callback<T, std::void_t<decltype(T::data)>> : std::is_convertible<decltype(T::data), raw_callback*> {};

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