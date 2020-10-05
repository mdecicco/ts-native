#pragma once
#include <common/types.h>
#include <builtin/builtin.h>
#include <util/template_utils.hpp>
#include <util/robin_hood.h>
#include <dyncall.h>

#include <vector>
#include <typeindex>
#include <tuple>
#include <string>

#define METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__))&cls::method)
#define CONST_METHOD_PTR(cls, method, ret, ...) ((ret(cls::*)(__VA_ARGS__) const)&cls::method)

namespace gjs {
    class vm_backend;
    class script_function;
    class script_type;
    class type_manager;

    class bind_exception : public std::exception {
        public:
            bind_exception(const std::string& _text) : text(_text) { }
            ~bind_exception() { }

            virtual char const* what() const { return text.c_str(); }

            std::string text;
    };

    namespace bind {
        struct wrapped_function {
            wrapped_function(std::type_index ret, std::vector<std::type_index> args, const std::string& _name)
            : return_type(ret), arg_types(args), name(_name), is_static_method(false), address(0), ret_is_ptr(false) { }

            std::string name;
            std::type_index return_type;
            std::vector<std::type_index> arg_types;
            std::vector<bool> arg_is_ptr;
            bool is_static_method;

            bool ret_is_ptr;
            u64 address;

            virtual void call(void* ret, void** args) = 0;
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
                name(_name), internal_name(_internal_name), size(_size), dtor(nullptr), requires_subtype(false)
            {
            }

            ~wrapped_class();

            std::string name;
            std::string internal_name;
            bool requires_subtype;
            std::vector<wrapped_function*> methods;
            robin_hood::unordered_map<std::string, property*> properties;
            wrapped_function* dtor;
            size_t size;
        };

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
        call_const_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
            (*self.*method)(args...);
        }

        template <typename Ret, typename... Args>
        struct global_function : wrapped_function {
            typedef Ret (*func_type)(Args...);
            typedef std::tuple_size<std::tuple<Args...>> arg_count;

            global_function(type_manager* tpm, func_type f, const std::string& name) :
                wrapped_function(
                    typeid(remove_all<Ret>::type),
                    { typeid(remove_all<Args>::type)... },
                    name
                ),
                original_func(f)
            {
                if (!tpm->get<Ret>()) {
                    throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
                }
                // describe the function for the wrapped_function interface
                ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                address = u64(reinterpret_cast<void*>(f));

                bool sbv_args[] = { std::is_class_v<Args>..., false };
                bool bt_args[] = { (!std::is_same_v<Args, script_type*> && tpm->get<Args>() == nullptr)..., false };
                // script_type* is allowed because host subclass types will be constructed with it, but
                // it should not be bound as a type

                for (u8 a = 0;a < arg_count::value;a++) {
                    if (sbv_args[a]) {
                        throw bind_exception(format(
                            "Argument %d of function '%s' is a struct or class that is passed by value. "
                            "This is unsupported. Please use reference or pointer types for structs/classes"
                        , a, name.c_str()));
                    }

                    if (bt_args[a]) {
                        throw bind_exception(format(
                            "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                        , a, name.c_str(), arg_types[a].name()));
                    }
                }
            }

            virtual void call(void* ret, void** args);

            func_type original_func;
        };

        template <typename Ret, typename Cls, typename... Args>
        struct class_method : wrapped_function {
            public:
                typedef Ret (Cls::*method_type)(Args...);
                typedef Ret (*func_type)(method_type, Cls*, Args...);
                typedef std::tuple_size<std::tuple<Args...>> ac;
                
                class_method(type_manager* tpm, method_type f, const std::string& name) :
                    wrapped_function(
                        typeid(remove_all<Ret>::type),
                        { typeid(remove_all<Cls>::type), typeid(remove_all<Args>::type)... },
                        name
                    ),
                    original_func(call_class_method<Ret, Cls, Args...>)
                {
                    if (!tpm->get<Cls>()) {
                        throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }

                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { true, (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    address = *(u64*)reinterpret_cast<void*>(&f);

                    bool sbv_args[] = { std::is_class_v<Args>..., false };
                    bool bt_args[] = { (!std::is_same_v<Args, script_type*> && tpm->get<Args>() == nullptr)..., false };
                    // script_type* is allowed because host subclass types will be constructed with it, but
                    // it should not be bound as a type

                    for (u8 a = 0;a < ac::value;a++) {
                        if (sbv_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is a struct or class that is passed by value. "
                                "This is unsupported. Please use reference or pointer types for structs/classes"
                            , a, name.c_str()));
                        }

                        if (bt_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                            , a, name.c_str(), arg_types[a].name()));
                        }
                    }
                }

                virtual void call(void* ret, void** args);

                func_type original_func;
        };

        template <typename Ret, typename Cls, typename... Args>
        struct const_class_method : wrapped_function {
            public:
                typedef Ret (Cls::*method_type)(Args...) const;
                typedef Ret (*func_type)(method_type, Cls*, Args...);
                typedef std::tuple_size<std::tuple<Args...>> ac;
                
                const_class_method(type_manager* tpm, method_type f, const std::string& name) :
                    wrapped_function(
                        typeid(remove_all<Ret>::type),
                        { typeid(remove_all<Cls>::type), typeid(remove_all<Args>::type)... },
                        name
                    ),
                    original_func(call_const_class_method<Ret, Cls, Args...>)
                {
                    if (!tpm->get<Cls>()) {
                        throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }

                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { true, (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    address = *(u64*)reinterpret_cast<void*>(&f);

                    bool sbv_args[] = { std::is_class_v<Args>..., false };
                    bool bt_args[] = { (!std::is_same_v<Args, script_type*> && tpm->get<Args>() == nullptr)..., false };
                    // script_type* is allowed because host subclass types will be constructed with it, but
                    // it should not be bound as a type

                    for (u8 a = 0;a < ac::value;a++) {
                        if (sbv_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is a struct or class that is passed by value. "
                                "This is unsupported. Please use reference or pointer types for structs/classes"
                            , a, name.c_str()));
                        }

                        if (bt_args[a]) {
                            throw bind_exception(format(
                                "Argument %d of function '%s' is of type '%s' that has not been bound yet"
                            , a, name.c_str(), arg_types[a].name()));
                        }
                    }
                }

                virtual void call(void* ret, void** args);

                func_type original_func;
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
        Cls* construct_object(Cls* mem, Args... args) {
            return new (mem) Cls(args...);
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
                script_type* tp = tpm->add(name, typeid(remove_all<Cls>::type).name());
                tp->is_host = true;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
            wrap_class& constructor() {
                requires_subtype = std::is_same_v<std::tuple_element_t<0, std::tuple<Args...>>, script_type*>;
                methods.push_back(wrap_constructor<Cls, Args...>(types, name));
                if (!dtor) dtor = wrap_destructor<Cls>(types, name);
                return *this;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
            wrap_class& constructor() {
                methods.push_back(wrap_constructor<Cls, Args...>(types, name));
                if (!dtor) dtor = wrap_destructor<Cls>(types, name);
                return *this;
            }

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...)) {
                methods.push_back(wrap(types, name + "::" + _name, func));
                return *this;
            }

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...) const) {
                methods.push_back(wrap(types, name + "::" + _name, func));
                return *this;
            }

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(*func)(Args...)) {
                methods.push_back(wrap(types, name + "::" + _name, func));
                methods[methods.size() - 1]->is_static_method = true;
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T Cls::*member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), offset, flags);
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T *member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), (u64)member, flags | pf_static);
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    wrap(types, name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)() const, T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    wrap(types, name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }


            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    property_flags::pf_read_only | flags
                );
                return *this;
            }


            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)() const, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    property_flags::pf_read_only | flags
                );
                return *this;
            }

            script_type* finalize() {
                return types->finalize_class(this);
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
            printf("0x%llX\n", p);
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
            printf("arg[%d]: ", i);
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

        template <typename Ret, typename... Args>
        void srv_wrapper(Ret* out, Ret (*f)(Args...), Args... args) {
            new (out) Ret(f(args...));
        }

        /*
         * Call helpers for VM
         * todo: move DCCallVM to script_context
         */
        template <typename Ret, typename... Args>
        void global_function<Ret, Args...>::call(void* ret, void** args) {
            DCCallVM* call = dcNewCallVM(4096);
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (argc > 0) {
                if constexpr (std::is_class_v<Ret>) {
                    void* _args[argc + 2] = { ret, original_func };
                    for (u8 a = 0;a < argc;a++) _args[a + 2] = args[a];
                    _pass_arg_wrapper<Ret*, void*, Args...>(0, call, _args);
                } else _pass_arg_wrapper<Args...>(0, call, args);
            } else if constexpr (std::is_class_v<Ret>) {
                dcArgPointer(call, ret);
                dcArgPointer(call, (void*)original_func);
            }


            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, original_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper<Ret, Args...>);
            } else {
                do_call<Ret>(call, (Ret*)ret, original_func);
            }
            dcFree(call);
        }
        
        template <typename Ret, typename Cls, typename... Args>
        void class_method<Ret, Cls, Args...>::call(void* ret, void** args) {
            DCCallVM* call = dcNewCallVM(4096);
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 4] = { ret, original_func, (void*)address };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 3] = args[a];
                // return value, original_func, method pointer, this obj
                _pass_arg_wrapper<Ret*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 2] = { (void*)address };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 1] = args[a];
                // method pointer, this obj
                _pass_arg_wrapper<void*, void*, Args...>(0, call, _args);
            }

            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, original_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper<Ret, void*, void*, Args...>);
            } else {
                do_call<Ret>(call, (Ret*)ret, original_func);
            }
            dcFree(call);
        }

        template <typename Ret, typename Cls, typename... Args>
        void const_class_method<Ret, Cls, Args...>::call(void* ret, void** args) {
            DCCallVM* call = dcNewCallVM(4096);
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 4] = { ret, original_func, (void*)address };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 3] = args[a];
                // return value, original_func, method pointer, this obj
                _pass_arg_wrapper<Ret*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 2] = { (void*)address };
                for (u8 a = 0;a < argc + 1;a++) _args[a + 1] = args[a];
                // method pointer, this obj
                _pass_arg_wrapper<void*, void*, Args...>(0, call, _args);
            }

            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, original_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper<Ret, void*, void*, Args...>);
            } else {
                do_call<Ret>(call, (Ret*)ret, original_func);
            }
            dcFree(call);
        }
    };
};
#undef fimm