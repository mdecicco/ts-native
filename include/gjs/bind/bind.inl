#pragma once

namespace gjs {
    // global_function
    namespace bind {
        template <typename Ret, typename... Args>
        global_function<Ret, Args...>::global_function<Ret, Args...>(type_manager* tpm, func_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous)
        {
            if (!tpm->get<Ret>()) {
                throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
            }
            // describe the function for the wrapped_function interface
            ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            if constexpr (std::is_class_v<Ret>) {
                auto srv = srv_wrapper<Ret, Args...>;
                srv_wrapper_func = reinterpret_cast<void*>(srv);
            }

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

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
    };

    // class_method
    namespace bind {
        template <typename Ret, typename Cls, typename... Args>
        class_method<Ret, Cls, Args...>::class_method<Ret, Cls, Args...>(type_manager* tpm, method_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous),
            wrapper(call_class_method<Ret, Cls, Args...>)
        {
            if (!anonymous && !tpm->get<Cls>()) {
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
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

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
    };

    // const_class_method
    namespace bind {
        template <typename Ret, typename Cls, typename... Args>
        const_class_method<Ret, Cls, Args...>::const_class_method<Ret, Cls, Args...>(type_manager* tpm, method_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous),
            wrapper(call_const_class_method<Ret, Cls, Args...>)
        {
            if (!anonymous && !tpm->get<Cls>()) {
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
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

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

    // function wrapping helpers
    namespace bind {
        template <typename Ret, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(*func)(Args...), bool anonymous) {
            return new global_function<Ret, Args...>(tpm, func, name, anonymous);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(Cls::*func)(Args...), bool anonymous) {
            return new class_method<Ret, Cls, Args...>(tpm, func, name, anonymous);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(Cls::*func)(Args...) const, bool anonymous) {
            return new const_class_method<Ret, Cls, Args...>(tpm, func, name, anonymous);
        };


        template <typename Cls, typename... Args>
        wrapped_function* wrap_constructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::constructor", construct_object<Cls, Args...>);
        }

        template <typename Cls>
        wrapped_function* wrap_destructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::destructor", destruct_object<Cls>);
        }
    };

    // wrap_class
    namespace bind {
        template <typename Cls>
        wrap_class<Cls>::wrap_class<Cls>(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(remove_all<Cls>::type).name(), sizeof(remove_all<Cls>::type)), types(tpm) {
            type = tpm->add(name, typeid(remove_all<Cls>::type).name());
            type->size = size;
            type->is_host = true;
            trivially_copyable = std::is_trivially_copyable_v<Cls>;
            is_pod = std::is_pod_v<Cls>;
            if constexpr (!std::is_trivially_copyable_v<Cls>) {
                pass_ret = copy_construct_object<Cls>;
            }
        }

        template <typename Cls>
        template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int>>
        wrap_class<Cls>& wrap_class<Cls>::constructor() {
            methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
            if (!dtor) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
            return *this;
        }

        template <typename Cls>
        template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int>>
        wrap_class<Cls>& wrap_class<Cls>::constructor() {
            methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
            if (!dtor) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
            return *this;
        }

        // non-const methods
        template <typename Cls>
        template <typename Ret, typename... Args>
        wrap_class<Cls>& wrap_class<Cls>::method(const std::string& _name, Ret(Cls::*func)(Args...)) {
            methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
            return *this;
        }

        // const methods
        template <typename Cls>
        template <typename Ret, typename... Args>
        wrap_class<Cls>& wrap_class<Cls>::method(const std::string& _name, Ret(Cls::*func)(Args...) const) {
            methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
            return *this;
        }

        // static methods
        template <typename Cls>
        template <typename Ret, typename... Args>
        wrap_class<Cls>& wrap_class<Cls>::method(const std::string& _name, Ret(*func)(Args...)) {
            methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
            methods[methods.size() - 1]->is_static_method = true;
            return *this;
        }

        // static method which can have 'this' obj passed to it
        template <typename Cls>
        template <typename Ret, typename... Args>
        wrap_class<Cls>& wrap_class<Cls>::method(const std::string& _name, Ret(*func)(Args...), bool pass_this) {
            methods.push_back(wrap(types->ctx()->types(), name + "::" + _name, func));
            methods[methods.size() - 1]->is_static_method = true;
            methods[methods.size() - 1]->pass_this = pass_this;
            return *this;
        }

        // normal member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T Cls::*member, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

            u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
            properties[_name] = new property(nullptr, nullptr, tp, offset, flags);
            return *this;
        }

        // static member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T *member, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

            properties[_name] = new property(nullptr, nullptr, tp, (u64)member, flags | pf_static);
            return *this;
        }

        // getter, setter member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

            properties[_name] = new property(
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                tp,
                0,
                flags
            );
            return *this;
        }

        // const getter, setter member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)() const, T(Cls::*setter)(T), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;

            properties[_name] = new property(
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                tp,
                0,
                flags
            );
            return *this;
        }

        // read only member with getter
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)(), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;
            else flags |= property_flags::pf_read_only;

            properties[_name] = new property(
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                nullptr,
                tp,
                0,
                flags
            );
            return *this;
        }

        // read only member with const getter
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)() const, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
            }

            script_type* tp = types->ctx()->type<T>();
            if (!tp) {
                throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
            }

            if (std::is_pointer_v<T>) flags |= property_flags::pf_pointer;
            else flags |= property_flags::pf_read_only;

            properties[_name] = new property(
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                nullptr,
                tp,
                0,
                flags
            );
            return *this;
        }

        template <typename Cls>
        script_type* wrap_class<Cls>::finalize(script_module* mod) {
            return types->finalize_class(this, mod);
        }
    };

    // pseudo_class
    namespace bind {
        template <typename prim>
        pseudo_class<prim>::pseudo_class<prim>(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(remove_all<prim>::type).name(), 0), types(tpm) {
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

        template <typename prim>
        template <typename Ret, typename... Args>
        pseudo_class<prim>& pseudo_class<prim>::method(const std::string& _name, Ret(*func)(prim, Args...)) {
            wrapped_function* f = wrap(types->ctx()->types(), name + "::" + _name, func);
            f->is_static_method = true;
            methods.push_back(f);
            return *this;
        }

        template <typename prim>
        script_type* pseudo_class<prim>::finalize(script_module* mod) {
            return types->finalize_class(this, mod);
        }
    };

    // callers
    namespace bind {
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
        inline void pass_arg(DCCallVM* call, std::enable_if_t<is_callback<T>::value, void*> p) {
            raw_callback* cb = (raw_callback*)p;
            dcArgPointer(call, cb);
        }

        template <typename T>
        inline void pass_arg(DCCallVM* call, std::enable_if_t<std::is_class_v<T> && !is_callback<T>::value, void*> p) {
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
        inline void do_call(DCCallVM* call, std::enable_if_t<std::is_pointer_v<T>, T>* ret, void* func) {
            *ret = (T)dcCallPointer(call, func);
        }

        /*
        * Call helpers for VM
        */

    };
};