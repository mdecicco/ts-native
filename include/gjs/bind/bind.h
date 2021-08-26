#pragma once
#include <gjs/common/types.h>
#include <gjs/common/function_signature.h>
#include <gjs/builtin/builtin.h>
#include <gjs/util/template_utils.hpp>
#include <gjs/util/robin_hood.h>
#include <dyncall.h>

#include <vector>
#include <typeindex>
#include <tuple>
#include <string>

#define FUNC_PTR(func, ret, ...) ((ret(*)(__VA_ARGS__))func)
#define METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__))&cls::method)
#define CONST_METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__) const)&cls::method)

namespace gjs {
    class vm_backend;
    class script_function;
    class script_type;
    class script_module;
    class type_manager;

    class bind_exception : public std::exception {
        public:
            bind_exception(const std::string& _text) : text(_text) { }
            ~bind_exception() { }

            virtual char const* what() const { return text.c_str(); }

            std::string text;
    };

    namespace bind {
        typedef void (*pass_ret_func)(void* /*dest*/, void* /*src*/, size_t /*size*/);
        void trivial_copy(void* dest, void* src, size_t sz);

        template <typename Ret, typename... Args>
        script_type* signature(script_context* ctx, Ret (*f)(Args...)) {
            script_type* rt = ctx->type<Ret>();
            if (!rt) throw bind_exception(format("Cannot construct signature type for function with unbound return type '%s'", base_type_name<Ret>()));
            constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
            script_type* argTps[argc] = { ctx->type<Args>()... };

            for (u8 i = 0;i < argc;i++) {
                if (!argTps[i]) {
                    throw bind_exception(format("Cannot construct signature type for function with unbound argument type (arg index %d)", i));
                }
            }

            return tpm->get(function_signature(
                ctx,
                rt,
                (std::is_reference_v<Ret> || std::is_pointer_v<Ret>),
                argTps,
                argc,
                null
            ));
        }

        template <typename Cls, typename Ret, typename... Args>
        script_type* signature(script_context* ctx, Ret (Cls::*f)(Args...)) {
            script_type* rt = ctx->type<Ret>();
            if (!rt) throw bind_exception(format("Cannot construct signature type for function with unbound return type '%s'", base_type_name<Ret>()));
            constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
            script_type* argTps[argc] = { ctx->type<Args>()... };

            for (u8 i = 0;i < argc;i++) {
                if (!argTps[i]) {
                    throw bind_exception(format("Cannot construct signature type for function with unbound argument type (arg index %d)", i));
                }
            }

            return tpm->get(function_signature(
                ctx,
                rt,
                (std::is_reference_v<Ret> || std::is_pointer_v<Ret>),
                argTps,
                argc,
                ctx->type<Cls>
            ));
        }

        struct wrapped_function {
            wrapped_function(std::type_index ret, std::vector<script_type*> args, const std::string& _name)
            : return_type(ret), arg_types(args), name(_name), is_static_method(false), func_ptr(nullptr), ret_is_ptr(false),
              srv_wrapper_func(nullptr), call_method_func(nullptr), pass_this(false) { }

            std::string name;
            std::type_index return_type;
            std::vector<bool> arg_is_ptr;
            std::vector<script_type*> arg_types;
            bool is_static_method;

            // whether or not to pass this obj when the method is static
            // (Useful for artificially extending types)
            bool pass_this;

            bool ret_is_ptr;

            // address of the host function to be called
            void* func_ptr;

            // class method: return value pointer, call_method_func, func_ptr, self, function args...
            // global function: return value pointer, func_ptr, function args...
            void* srv_wrapper_func;

            // func_ptr, self, method args...
            void* call_method_func;

            virtual void call(DCCallVM* call, void* ret, void** args) = 0;
        };

        struct wrapped_class {
            struct property {
                property(wrapped_function* g, wrapped_function* s, std::type_index t, u64 o, u8 f) :
                    getter(g), setter(s), type(t), offset(o), flags(f) { }

                wrapped_function* getter;
                wrapped_function* setter;
                std::type_index type;
                u64 offset;
                u8 flags;
            };

            wrapped_class(const std::string& _name, const std::string& _internal_name, size_t _size) :
                name(_name), internal_name(_internal_name), size(_size), dtor(nullptr),
                trivially_copyable(true), pass_ret(trivial_copy), type(nullptr), is_pod(false)
            {
            }

            ~wrapped_class();

            std::string name;
            std::string internal_name;
            bool is_pod;
            bool trivially_copyable;
            pass_ret_func pass_ret;
            std::vector<wrapped_function*> methods;
            robin_hood::unordered_map<std::string, property*> properties;
            wrapped_function* dtor;
            script_type* type;
            size_t size;
        };

        // call wrapper for functions which return structs by value
        template <typename Ret, typename... Args>
        void srv_wrapper(Ret* out, Ret (*f)(Args...), Args... args) {
            new (out) Ret(f(args...));
        }

        // Call class method on object
        // Only visible to class methods with non-void return
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
            return (*self.*method)(args...);
        }
        
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        call_const_class_method(Ret(Cls::*method)(Args...) const, Cls* self, Args... args) {
            return (*self.*method)(args...);
        }

        // Call class method on object
        // Only visible to class methods with void return
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
            (*self.*method)(args...);
        }

        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        call_const_class_method(Ret(Cls::*method)(Args...) const, Cls* self, Args... args) {
            (*self.*method)(args...);
        }

        template <typename Ret, typename... Args>
        struct global_function : wrapped_function {
            typedef Ret (*func_type)(Args...);
            typedef std::tuple_size<std::tuple<Args...>> arg_count;

            global_function(type_manager* tpm, func_type f, const std::string& name) :
                wrapped_function(
                    typeid(remove_all<Ret>::type),
                    { tpm->get<Args>()... },
                    name
                )
            {
                if (!tpm->get<Ret>()) {
                    throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
                }
                // describe the function for the wrapped_function interface
                ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                func_ptr = *reinterpret_cast<void**>(&f);

                if constexpr (std::is_class_v<Ret>) {
                    auto srv = srv_wrapper<Ret, Args...>;
                    srv_wrapper_func = reinterpret_cast<void*>(srv);
                }

                bool sbv_args[] = { std::is_class_v<Args>..., false };

                for (u8 a = 0;a < arg_count::value;a++) {
                    if (sbv_args[a]) {
                        throw bind_exception(format(
                            "Argument %d of function '%s' is a struct or class that is passed by value. "
                            "This is unsupported. Please use reference or pointer types for structs/classes"
                        , a, name.c_str()));
                    }

                    if (!arg_types[a]) {
                        const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                        throw bind_exception(format(
                            "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                        , a, name.c_str(), arg_typenames[a]));
                    }
                }
            }

            virtual void call(DCCallVM* call, void* ret, void** args);
        };

        template <typename Ret, typename Cls, typename... Args>
        struct class_method : wrapped_function {
            public:
                typedef Ret (Cls::*method_type)(Args...);
                typedef std::tuple_size<std::tuple<Args...>> ac;
                typedef Ret (*func_type)(method_type, Cls*, Args...);

                func_type wrapper;
                
                class_method(type_manager* tpm, method_type f, const std::string& name) :
                    wrapped_function(
                        typeid(remove_all<Ret>::type),
                        { tpm->get<Args>()... },
                        name
                    ),
                    wrapper(call_class_method<Ret, Cls, Args...>)
                {
                    if (!tpm->get<Cls>()) {
                        throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }

                    call_method_func = wrapper;
                    if constexpr (std::is_class_v<Ret>) {
                        auto srv = srv_wrapper<Ret, method_type, Cls*, Args...>;
                        srv_wrapper_func = reinterpret_cast<void*>(srv);
                    }

                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    func_ptr = *reinterpret_cast<void**>(&f);

                    bool sbv_args[] = { std::is_class_v<Args>..., false };

                    for (u8 a = 0;a < ac::value;a++) {
                        if (sbv_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is a struct or class that is passed by value. "
                                "This is unsupported. Please use reference or pointer types for structs/classes"
                            , a, name.c_str()));
                        }

                        if (!arg_types[a]) {
                            const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                            throw bind_exception(format(
                                "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                            , a, name.c_str(), arg_typenames[a]));
                        }
                    }
                }

                virtual void call(DCCallVM* call, void* ret, void** args);
        };

        template <typename Ret, typename Cls, typename... Args>
        struct const_class_method : wrapped_function {
            public:
                typedef Ret (Cls::*method_type)(Args...) const;
                typedef std::tuple_size<std::tuple<Args...>> ac;
                typedef Ret (*func_type)(method_type, Cls*, Args...);

                func_type wrapper;
                
                const_class_method(type_manager* tpm, method_type f, const std::string& name) :
                    wrapped_function(
                        typeid(remove_all<Ret>::type),
                        { tpm->get<Args>()... },
                        name
                    ),
                    wrapper(call_const_class_method<Ret, Cls, Args...>)
                {
                    if (!tpm->get<Cls>()) {
                        throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }

                    call_method_func = wrapper;
                    if constexpr (std::is_class_v<Ret>) {
                        auto srv = srv_wrapper<Ret, method_type, Cls*, Args...>;
                        srv_wrapper_func = reinterpret_cast<void*>(srv);
                    };

                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    func_ptr = *reinterpret_cast<void**>(&f);

                    bool sbv_args[] = { std::is_class_v<Args>..., false };

                    for (u8 a = 0;a < ac::value;a++) {
                        if (sbv_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is a struct or class that is passed by value. "
                                "This is unsupported. Please use reference or pointer types for structs/classes"
                            , a, name.c_str()));
                        }

                        if (!arg_types[a]) {
                            const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                            throw bind_exception(format(
                                "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                            , a, name.c_str(), arg_typenames[a]));
                        }
                    }
                }

                virtual void call(DCCallVM* call, void* ret, void** args);
        };

        /*
         * Function wrapping helper
         */
        template <typename Ret, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(*func)(Args...)) {
            return new global_function<Ret, Args...>(tpm, func, name);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(Cls::*func)(Args...)) {
            return new class_method<Ret, Cls, Args...>(tpm, func, name);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(Cls::*func)(Args...) const) {
            return new const_class_method<Ret, Cls, Args...>(tpm, func, name);
        };


        /*
         * Class wrapping helper
         */
        template <typename Cls, typename... Args>
        void construct_object(Cls* mem, Args... args) {
            new (mem) Cls(args...);
        }

        template <typename Cls>
        void copy_construct_object(void* dest, void* src, size_t sz) {
            new ((Cls*)dest) Cls(*(Cls*)src);
        }

        template <typename Cls>
        void destruct_object(Cls* obj) {
            obj->~Cls();
        }

        template <typename Cls, typename... Args>
        wrapped_function* wrap_constructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::constructor", construct_object<Cls, Args...>);
        }

        template <typename Cls>
        wrapped_function* wrap_destructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::destructor", destruct_object<Cls>);
        }

        enum property_flags {
            pf_none             = 0b00000000,
            pf_read_only        = 0b00000001,
            pf_write_only       = 0b00000010,
            pf_pointer          = 0b00000100,
            pf_static           = 0b00001000
        };

        template <typename Cls>
        struct wrap_class : wrapped_class {
            wrap_class(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(remove_all<Cls>::type).name(), sizeof(remove_all<Cls>::type)), types(tpm) {
                type = tpm->add(name, typeid(remove_all<Cls>::type).name());
                type->size = size;
                type->is_host = true;
                trivially_copyable = std::is_trivially_copyable_v<Cls>;
                is_pod = std::is_pod_v<Cls>;
                if constexpr (!std::is_trivially_copyable_v<Cls>) {
                    pass_ret = copy_construct_object<Cls>;
                }
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
            wrap_class& constructor() {
                methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
                if (!dtor) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
                return *this;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
            wrap_class& constructor() {
                methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
                if (!dtor) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
                return *this;
            }

            // non-const methods
            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...)) {
                methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
                return *this;
            }

            // const methods
            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...) const) {
                methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
                return *this;
            }

            // static methods
            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(*func)(Args...)) {
                methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
                methods[methods.size() - 1]->is_static_method = true;
                return *this;
            }

            // static method which can have 'this' obj passed to it
            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(*func)(Args...), bool pass_this) {
                methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
                methods[methods.size() - 1]->is_static_method = true;
                methods[methods.size() - 1]->pass_this = pass_this;
                return *this;
            }

            // normal member
            template <typename T>
            wrap_class& prop(const std::string& _name, T Cls::*member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

                u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), offset, flags);
                return *this;
            }

            // static member
            template <typename T>
            wrap_class& prop(const std::string& _name, T *member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), (u64)member, flags | pf_static);
                return *this;
            }

            // getter, setter member
            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

                properties[_name] = new property(
                    wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                    wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            // const getter, setter member
            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)() const, T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }


                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

                properties[_name] = new property(
                    wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                    wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            // read only member with getter
            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;
                else flags |= property_flags::pf_read_only;

                properties[_name] = new property(
                    wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            // read only member with const getter
            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)() const, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->ctx()->types()->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;
                else flags |= property_flags::pf_read_only;

                properties[_name] = new property(
                    wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            script_type* finalize(script_module* mod) {
                return types->finalize_class(this, mod);
            }

            type_manager* types;
        };

        template <typename prim>
        struct pseudo_class : wrapped_class {
            pseudo_class(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(remove_all<prim>::type).name(), 0), types(tpm) {
                type = tpm->add(name, typeid(remove_all<prim>::type).name());
                if constexpr (!std::is_same_v<void, prim>) size = sizeof(prim);
                trivially_copyable = true;
                is_pod = true;
                type->size = size;
                type->is_host = true;
                type->is_primitive = true;
                type->is_pod = true;
                type->is_trivially_copyable = true;
            }

            template <typename Ret, typename... Args>
            pseudo_class& method(const std::string& _name, Ret(*func)(prim, Args...)) {
                wrapped_function* f = wrap(types->ctx()->types(), name + "::" + _name, func);
                f->is_static_method = true;
                methods.push_back(f);
                return *this;
            }

            script_type* finalize(script_module* mod) {
                return types->finalize_class(this, mod);
            }

            type_manager* types;
        };
        

        #define pa_func(tp) template <typename T> inline void pass_arg(DCCallVM* call, std::enable_if_t<std::is_same_v<T, tp>, void*> p)
        #define pa_func_simp(tp, func) pa_func(tp) { func(call, *(tp*)&p); }
        
        pa_func_simp(bool, dcArgBool);
        pa_func_simp(f32, dcArgFloat);
        pa_func_simp(f64, dcArgDouble)
        pa_func_simp(char, dcArgChar);
        pa_func_simp(u8, dcArgChar)
        pa_func_simp(i8, dcArgChar)
        pa_func_simp(u16, dcArgShort)
        pa_func_simp(i16, dcArgShort)
        pa_func_simp(u32, dcArgLong)
        pa_func_simp(i32, dcArgInt)
        pa_func_simp(u64, dcArgLongLong)
        pa_func_simp(i64, dcArgLongLong)

        #undef pa_func
        #undef pa_func_simp
        
        template <typename T>
        inline void pass_arg(DCCallVM* call, std::enable_if_t<std::is_pointer_v<T> || std::is_reference_v<T>, void*> p) {
            dcArgPointer(call, p);
        }

        template <typename T>
        inline void pass_arg(DCCallVM* call, std::enable_if_t<std::is_class_v<T>, void*> p) {
            // This only exists to prevent confusing compiler errors.
            // A bind_exception will be thrown when binding any function
            // that has a struct or class passed by value
        }

        template <typename T, typename... Rest>
        void _pass_arg_wrapper(u16 i, DCCallVM* call, void** params) {
            pass_arg<T>(call, params[i]);
            if constexpr (std::tuple_size_v<std::tuple<Rest...>> > 0) {
                _pass_arg_wrapper<Rest...>(i + 1, call, params);
            }
        }

        template <typename T>
        void do_call(DCCallVM* call, std::enable_if_t<!std::is_pointer_v<T>, T>* ret, void* func);

        template <typename T>
        inline void do_call(DCCallVM* call, std::enable_if_t<std::is_pointer_v<T>, T>* ret, void* func) {
            *ret = (T)dcCallPointer(call, func);
        }

        #define dc_func_simp(tp, cfunc) template <> void do_call<tp>(DCCallVM* call, tp* ret, void* func);
        dc_func_simp(f32, dcCallFloat);
        dc_func_simp(f64, dcCallDouble);
        dc_func_simp(bool, dcCallBool);
        dc_func_simp(u8, dcCallChar);
        dc_func_simp(i8, dcCallChar);
        dc_func_simp(u16, dcCallShort);
        dc_func_simp(i16, dcCallShort);
        dc_func_simp(u32, dcCallInt);
        dc_func_simp(i32, dcCallInt);
        dc_func_simp(u64, dcCallLongLong);
        dc_func_simp(i64, dcCallLongLong);
        #undef dc_func_simp
        
        template <> void do_call<void>(DCCallVM* call, void* ret, void* func);

        /*
         * Call helpers for VM
         */
        template <typename Ret, typename... Args>
        void global_function<Ret, Args...>::call(DCCallVM* call, void* ret, void** args) {
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (argc > 0) {
                if constexpr (std::is_class_v<Ret>) {
                    void* _args[argc + 2] = { ret, func_ptr };
                    for (u8 a = 0;a < argc;a++) _args[a + 2] = args[a];
                    _pass_arg_wrapper<Ret*, void*, Args...>(0, call, _args);
                } else _pass_arg_wrapper<Args...>(0, call, args);
            } else if constexpr (std::is_class_v<Ret>) {
                dcArgPointer(call, ret);
                dcArgPointer(call, (void*)func_ptr);
            }


            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, func_ptr);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper_func);
            } else {
                do_call<Ret>(call, (Ret*)ret, func_ptr);
            }
        }
        
        template <typename Ret, typename Cls, typename... Args>
        void class_method<Ret, Cls, Args...>::call(DCCallVM* call, void* ret, void** args) {
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 4] = { ret, call_method_func, func_ptr };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 3] = args[a];
                // return value, call_method_func, func_ptr, this obj
                _pass_arg_wrapper<Ret*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 2] = { func_ptr };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 1] = args[a];
                // func_ptr, this obj
                _pass_arg_wrapper<void*, void*, Args...>(0, call, _args);
            }

            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, call_method_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper_func);
            } else {
                do_call<Ret>(call, (Ret*)ret, call_method_func);
            }
        }

        template <typename Ret, typename Cls, typename... Args>
        void const_class_method<Ret, Cls, Args...>::call(DCCallVM* call, void* ret, void** args) {
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 4] = { ret, call_method_func, func_ptr };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 3] = args[a];
                // return value, call_method_func, func_ptr, this obj
                _pass_arg_wrapper<Ret*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 2] = { func_ptr };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 1] = args[a];
                // func_ptr, this obj
                _pass_arg_wrapper<void*, void*, Args...>(0, call, _args);
            }

            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, call_method_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper_func);
            } else {
                do_call<Ret>(call, (Ret*)ret, call_method_func);
            }
        }
    };
};
#undef fimm