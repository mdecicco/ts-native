#pragma once
#include <gjs/common/errors.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/exec_context.h>
#include <gjs/util/typeof.h>

namespace gjs {
    // function call wrappers
    namespace ffi {
        template <typename Ret, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        cdecl_func(Ret (*f)(Args...), exec_context* ectx, Args... args) {
            try {
                return f(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

        // Void cdecl wrapper
        template <typename Ret, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        cdecl_func(Ret (*f)(Args...), exec_context* ectx, Args... args) {
            try {
                f(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

        template <typename Ret, typename... Args>
        void srv_wrapper(Ret* out, Ret (*f)(Args...), Args... args) {
            new (out) Ret(f(args...));
        }

        // Non-const methods
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), exec_context* ectx, Cls* self, Args... args) {
            try {
                return (*self.*method)(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

        // Const methods
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        call_const_class_method(Ret(Cls::*method)(Args...) const, exec_context* ectx, Cls* self, Args... args) {
            try {
                return (*self.*method)(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

        // Non-const methods
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), exec_context* ectx, Cls* self, Args... args) {
            try {
                (*self.*method)(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

        // Const methods
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        call_const_class_method(Ret(Cls::*method)(Args...) const, exec_context* ectx, Cls* self, Args... args) {
            try {
                (*self.*method)(args...);
            } catch (const std::exception& e) {
                ectx->trace.produce_error(e.what());
            }
        }

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
    };

    // global_function
    namespace ffi {
        template <typename Ret, typename... Args>
        global_function<Ret, Args...>::global_function(type_manager* tpm, func_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous)
        {
            if (!return_type) {
                throw error::bind_exception(error::ecode::b_function_return_type_unbound, base_type_name<Ret>(), name.c_str());
            }
            // describe the function for the wrapped_function interface
            ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            if constexpr (std::is_class_v<Ret>) {
                auto srv = srv_wrapper<Ret, func_type, exec_context*, Args...>;
                srv_wrapper_func = reinterpret_cast<void*>(srv);
            }

            auto cw = cdecl_func<Ret, Args...>;
            cdecl_wrapper_func = reinterpret_cast<void*>(cw);

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

            if constexpr (arg_count::value > 0) {
                for (u8 a = 0;a < arg_count::value;a++) {
                    if (sbv_args[a]) {
                        throw error::bind_exception(error::ecode::b_arg_struct_pass_by_value, a, name.c_str());
                    }

                    if (!arg_types[a]) {
                        const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                        throw error::bind_exception(error::ecode::b_arg_type_unbound, a, name.c_str(), arg_typenames[a]);
                    }
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
                    void* _args[argc + 4] = { ret, cdecl_wrapper_func, func_ptr };
                    for (u8 a = 0;a < argc + 1;a++) _args[a + 3] = args[a];
                    // return value, cdecl_wrapper_func, func_ptr, exec_ctx, ...
                    _pass_arg_wrapper<Ret*, void*, void*, void*, Args...>(0, call, _args);
                } else {
                    void* _args[argc + 2] = { func_ptr };
                    for (u8 a = 0;a < argc + 1;a++) _args[a + 1] = args[a];
                    // func_ptr, exec_ctx, ...
                    _pass_arg_wrapper<void*, void*, Args...>(0, call, _args);
                }
            } else if constexpr (std::is_class_v<Ret>) {
                // return value, cdecl_wrapper_func, func_ptr, exec_ctx
                dcArgPointer(call, ret);
                dcArgPointer(call, (void*)cdecl_wrapper_func);
                dcArgPointer(call, (void*)func_ptr);
                dcArgPointer(call, args[0]);
            } else {
                // func_ptr, exec_ctx
                dcArgPointer(call, (void*)func_ptr);
                dcArgPointer(call, args[0]);
            }

            if constexpr (std::is_pointer_v<Ret> || std::is_reference_v<Ret>) {
                do_call<remove_all<Ret>*>(call, (remove_all<Ret>**)ret, cdecl_wrapper_func);
            } else if constexpr (std::is_class_v<Ret>) {
                do_call<void>(call, nullptr, srv_wrapper_func);
            } else {
                do_call<Ret>(call, (Ret*)ret, cdecl_wrapper_func);
            }
        }
    };

    // class_method
    namespace ffi {
        template <typename Ret, typename Cls, typename... Args>
        class_method<Ret, Cls, Args...>::class_method(type_manager* tpm, method_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous),
            wrapper(call_class_method<Ret, Cls, Args...>)
        {
            if (!anonymous && !tpm->get<Cls>()) {
                throw error::bind_exception(error::ecode::b_method_class_unbound, name.c_str(), base_type_name<Cls>());
            }
            if (!return_type) {
                throw error::bind_exception(error::ecode::b_method_return_type_unbound, base_type_name<Ret>(), base_type_name<Cls>(), name.c_str());
            }

            call_method_func = reinterpret_cast<void*>(wrapper);
            if constexpr (std::is_class_v<Ret>) {
                auto srv = srv_wrapper<Ret, method_type, exec_context*, Cls*, Args...>;
                srv_wrapper_func = reinterpret_cast<void*>(srv);
            }

            // describe the function for the wrapped_function interface
            ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

            if constexpr (ac::value > 0) {
                for (u8 a = 0;a < ac::value;a++) {
                    if (sbv_args[a]) {
                        throw error::bind_exception(error::ecode::b_method_arg_struct_pass_by_value, a, base_type_name<Cls>(), name.c_str());
                    }

                    if (!arg_types[a]) {
                        const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                        throw error::bind_exception(error::ecode::b_method_arg_type_unbound, a, base_type_name<Cls>(), name.c_str(), arg_typenames[a]);
                    }
                }
            }
        }

        template <typename Ret, typename Cls, typename... Args>
        void class_method<Ret, Cls, Args...>::call(DCCallVM* call, void* ret, void** args) {
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 5] = { ret, call_method_func, func_ptr };
                for (u8 a = 0;a < argc + 2;a++) _args[a + 3] = args[a];
                // return value, call_method_func, func_ptr, exec ctx, this obj, ...
                _pass_arg_wrapper<Ret*, void*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 3] = { func_ptr };
                for (u8 a = 0;a < argc + 2;a++) _args[a + 1] = args[a];
                // func_ptr, exec ctx, this obj, ...
                _pass_arg_wrapper<void*, void*, void*, Args...>(0, call, _args);
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
    namespace ffi {
        template <typename Ret, typename Cls, typename... Args>
        const_class_method<Ret, Cls, Args...>::const_class_method(type_manager* tpm, method_type f, const std::string& name, bool anonymous) :
            wrapped_function(tpm->get<Ret>(), { tpm->get<Args>()... }, name, anonymous),
            wrapper(call_const_class_method<Ret, Cls, Args...>)
        {
            if (!anonymous && !tpm->get<Cls>()) {
                throw error::bind_exception(error::ecode::b_method_class_unbound, name.c_str(), base_type_name<Cls>());
            }
            if (!return_type) {
                throw error::bind_exception(error::ecode::b_method_return_type_unbound, base_type_name<Ret>(), base_type_name<Cls>(), name.c_str());
            }

            call_method_func = reinterpret_cast<void*>(wrapper);
            if constexpr (std::is_class_v<Ret>) {
                auto srv = srv_wrapper<Ret, method_type, exec_context*, Cls*, Args...>;
                srv_wrapper_func = reinterpret_cast<void*>(srv);
            };

            // describe the function for the wrapped_function interface
            ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
            arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args> || is_callback<Args>::value)... };
            func_ptr = *reinterpret_cast<void**>(&f);

            bool sbv_args[] = { (std::is_class_v<Args> && !is_callback<Args>::value)..., false };

            if constexpr (ac::value > 0) {
                for (u8 a = 0;a < ac::value;a++) {
                    if (sbv_args[a]) {
                        throw error::bind_exception(error::ecode::b_method_arg_struct_pass_by_value, a, base_type_name<Cls>(), name.c_str());
                    }

                    if (!arg_types[a]) {
                        const char* arg_typenames[] = { base_type_name<Args>()..., nullptr };
                        throw error::bind_exception(error::ecode::b_method_arg_type_unbound, a, base_type_name<Cls>(), name.c_str(), arg_typenames[a]);
                    }
                }
            }
        }

        template <typename Ret, typename Cls, typename... Args>
        void const_class_method<Ret, Cls, Args...>::call(DCCallVM* call, void* ret, void** args) {
            dcMode(call, DC_CALL_C_DEFAULT);
            dcReset(call);

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            if constexpr (std::is_class_v<Ret>) {
                void* _args[argc + 5] = { ret, call_method_func, func_ptr };
                for (u8 a = 0;a < argc + 2;a++) _args[a + 3] = args[a];
                // return value, call_method_func, func_ptr, exec ctx, this obj, ...
                _pass_arg_wrapper<Ret*, void*, void*, void*, void*, Args...>(0, call, _args);
            } else {
                void* _args[argc + 3] = { func_ptr };
                for (u8 a = 0;a < argc + 2;a++) _args[a + 1] = args[a];
                // func_ptr, exec ctx, this obj, ...
                _pass_arg_wrapper<void*, void*, void*, Args...>(0, call, _args);
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
    namespace ffi {
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
            auto ret = wrap(tpm, name + "::constructor", construct_object<Cls, Args...>);

            // And explicit 'this' pointer because gjs implicitly adds it later
            ret->arg_types.erase(ret->arg_types.begin());
            ret->arg_is_ptr.erase(ret->arg_is_ptr.begin());

            return ret;
        }

        template <typename Cls>
        wrapped_function* wrap_destructor(type_manager* tpm, const std::string& name) {
            auto ret = wrap(tpm, name + "::destructor", destruct_object<Cls>);

            // Remove explicit 'this' pointer for the same reason. This is the
            // only parameter to destruct_object, just clear the args
            ret->arg_types.clear();
            ret->arg_is_ptr.clear();

            return ret;
        }
    };

    // wrap_class
    namespace ffi {
        template <typename Cls>
        wrap_class<Cls>::wrap_class(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(typename remove_all<Cls>::type).name(), sizeof(typename remove_all<Cls>::type)), types(tpm) {
            type = tpm->add(name, typeid(typename remove_all<Cls>::type).name());
            type->size = u32(size);
            type->is_host = true;
            trivially_copyable = std::is_trivially_copyable_v<Cls>;
            is_pod = std::is_pod_v<Cls>;
        }

        template <typename Cls>
        template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int>>
        wrap_class<Cls>& wrap_class<Cls>::constructor() {
            methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
            if (!dtor && !std::is_trivially_destructible_v<Cls>) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
            return *this;
        }

        template <typename Cls>
        template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int>>
        wrap_class<Cls>& wrap_class<Cls>::constructor() {
            methods.push_back(wrap_constructor<Cls, Args...>(types->ctx()->types(), name));
            if (!dtor && !std::is_trivially_destructible_v<Cls>) dtor = wrap_destructor<Cls>(types->ctx()->types(), name);
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
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);

            u64 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
            
            property* p = new property(_name, nullptr, nullptr, tp, offset, flags);
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        // static member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T *member, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);

            property* p = new property(_name, nullptr, nullptr, tp, (u64)member, flags | u8(property_flags::pf_static));
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        // getter, setter member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);


            property* p = new property(
                _name,
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        // const getter, setter member
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)() const, T(Cls::*setter)(T), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);

            property* p = new property(
                _name,
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                wrap(types->ctx()->types(), name + "::set_" + _name, setter),
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);

            return *this;
        }

        // read only member with getter
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)(), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);
            else flags |= u8(property_flags::pf_read_only);

            property* p = new property(
                _name,
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                nullptr,
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        // read only member with const getter
        template <typename Cls>
        template <typename T>
        wrap_class<Cls>& wrap_class<Cls>::prop(const std::string& _name, T(Cls::*getter)() const, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);
            else flags |= u8(property_flags::pf_read_only);

            property* p = new property(
                _name,
                wrap(types->ctx()->types(), name + "::get_" + _name, getter),
                nullptr,
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        template <typename Cls>
        script_type* wrap_class<Cls>::finalize(script_module* mod) {
            return types->finalize_class(this, mod);
        }
    };

    // pseudo_class
    namespace ffi {
        template <typename prim>
        pseudo_class<prim>::pseudo_class(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(typename remove_all<prim>::type).name(), 0), types(tpm) {
            type = tpm->add(name, typeid(typename remove_all<prim>::type).name());
            if constexpr (!std::is_same_v<void, prim>) size = sizeof(prim);
            trivially_copyable = true;
            is_pod = true;
            type->size = u32(size);
            type->is_host = true;
            type->is_primitive = true;
            type->is_pod = true;
            type->is_trivially_copyable = true;
        }

        template <typename prim>
        template <typename Ret, typename... Args>
        pseudo_class<prim>& pseudo_class<prim>::method(const std::string& _name, Ret(*func)(prim, Args...)) {
            wrapped_function* f = wrap(types->ctx()->types(), name + "::" + _name, func);
            f->pass_this = true;
            methods.push_back(f);
            return *this;
        }

        template <typename prim>
        template <typename Ret, typename... Args>
        pseudo_class<prim>& pseudo_class<prim>::method(const std::string& _name, Ret(*func)(Args...)) {
            wrapped_function* f = wrap(types->ctx()->types(), name + "::" + _name, func);
            f->is_static_method = true;
            methods.push_back(f);
            return *this;
        }

        // read only member with getter
        template <typename prim>
        template <typename T>
        pseudo_class<prim>& pseudo_class<prim>::prop(const std::string& _name, T(*getter)(prim), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);
            else flags |= u8(property_flags::pf_read_only);

            auto get = wrap(types->ctx()->types(), name + "::get_" + _name, getter);
            get->pass_this = true;

            property* p = new property(
                _name,
                get,
                nullptr,
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        // getter, setter member
        template <typename prim>
        template <typename T>
        pseudo_class<prim>& pseudo_class<prim>::prop(const std::string& _name, T(*getter)(prim), T(*setter)(prim, T), u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);

            auto get = wrap(types->ctx()->types(), name + "::get_" + _name, getter);
            get->pass_this = true;
            auto set = wrap(types->ctx()->types(), name + "::set_" + _name, setter);
            set->pass_this = true;

            property* p = new property(
                _name,
                get,
                set,
                tp,
                0,
                flags
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        template <typename prim>
        template <typename T>
        std::enable_if_t<!std::is_function_v<T>, pseudo_class<prim>&>
        pseudo_class<prim>::prop(const std::string& _name, T *member, u8 flags) {
            if (properties.find(_name) != properties.end()) {
                throw error::bind_exception(error::ecode::b_prop_already_bound, _name.c_str(), name.c_str());
            }

            script_type* tp = type_of<T>(types->ctx());
            if (!tp) {
                throw error::bind_exception(error::ecode::b_prop_type_unbound, base_type_name<T>());
            }

            if (std::is_pointer_v<T>) flags |= u8(property_flags::pf_pointer);

            property* p = new property(
                _name,
                nullptr,
                nullptr,
                tp,
                (u64)member,
                flags | u8(property_flags::pf_static)
            );
            properties[_name] = p;
            ordered_props.push_back(p);
            return *this;
        }

        template <typename prim>
        script_type* pseudo_class<prim>::finalize(script_module* mod) {
            return types->finalize_class(this, mod);
        }
    };

    // callers
    namespace ffi {
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
    };
};