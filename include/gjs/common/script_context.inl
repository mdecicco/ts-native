#pragma once

namespace gjs {
    template <class Cls>
    std::enable_if_t<std::is_class_v<Cls>, bind::wrap_class<Cls>&>
    script_context::bind(const std::string& name) {
        return m_global->bind<Cls>(name);
    }

    template <class prim>
    std::enable_if_t<!std::is_class_v<prim> && !std::is_same_v<prim, void>, bind::pseudo_class<prim>&>
    script_context::bind(const std::string& name) {
        return m_global->bind<prim>(name);
    }

    template <typename Ret, typename... Args>
    script_function* script_context::bind(Ret(*func)(Args...), const std::string& name) {
        return m_global->bind<Ret, Args...>(func, name);
    }

    template <typename... Args>
    script_object script_context::instantiate(script_type* type, Args... args) {
        return script_object(this, type, args...);
    }

    template <typename... Args>
    script_object script_context::construct_at(script_type* type, void* dest, Args... args) {
        return script_object(this, type, (u8*)dest, args...);
    }

    template <typename T>
    script_type* script_context::type(bool do_throw) {
        return m_all_types->get<T>(do_throw);
    }

    template <typename... Args>
    script_object script_context::call(script_function* func, void* self, Args... args) {
        // validate signature
        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = (self != nullptr) == func->is_thiscall;

        if (!valid_call) {
            // todo
            if (self) {
                // self for non-thiscall exception
            } else {
                // no self for thiscall exception
            }
            return script_object(this);
        }

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(this, args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->type->signature->explicit_argc != ac) valid_call = false;

                for (u8 a = 0;a < func->type->signature->explicit_argc && valid_call;a++) {
                    valid_call = (func->type->signature->explicit_arg(a).tp->id() == arg_types[a]->id());
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(this);
            }

            std::vector<void*> vargs = { to_arg(args)... };
            if (self) vargs.insert(vargs.begin(), self);

            script_object out = script_object(this, (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.m_self = new u8[func->type->signature->return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, vargs.data());
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback::destroy((raw_callback**)vargs[a]);
                }
            }
            return out;
        } else {
            if (func->type->signature->explicit_argc != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(this);
            }

            script_object out = script_object(this, (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.m_self = new u8[func->type->signature->return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, &self);
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    raw_callback* cb = *cbp;
                    if (cb->owns_func && cb->ptr) {
                        delete cb->ptr->target->access.wrapped;
                        delete cb->ptr->target;
                    }

                    if (cb->owns_ptr && cb->ptr) delete cb->ptr;
                    delete cb;
                    if (cb->free_self) delete (void*)cbp;
                }
            }
            return out;
        }

        return script_object(this);
    }

    template <typename... Args>
    script_object script_context::call_callback(raw_callback* cb, Args... args) {
        script_function* func = cb->ptr->target;
        // validate signature
        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = true;

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(this, args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->type->signature->explicit_argc != ac) valid_call = false;

                for (u8 a = 0;a < func->type->signature->explicit_argc && valid_call;a++) {
                    valid_call = (func->type->signature->explicit_arg(a).tp->id() == arg_types[a]->id());
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(this);
            }

            std::vector<void*> vargs = { cb->ptr->data, to_arg(args)... };
            if (cb->ptr->self_obj()) vargs.insert(vargs.begin(), cb->ptr->self_obj());

            script_object out = script_object(this, (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.m_self = new u8[func->type->signature->return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, vargs.data());
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    raw_callback* cb = *cbp;
                    if (cb->owns_func && cb->ptr) {
                        delete cb->ptr->target->access.wrapped;
                        delete cb->ptr->target;
                    }

                    if (cb->owns_ptr && cb->ptr) delete cb->ptr;
                    delete cb;
                    if (cb->free_self) delete (void*)cbp;
                }
            }
            return out;
        } else {
            if (func->type->signature->explicit_argc != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(this);
            }

            script_object out = script_object(this, (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.m_self = new u8[func->type->signature->return_type->size];
                out.m_owns_ptr = true;
            }
            m_backend->call(func, out.m_self, &self);
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    raw_callback* cb = *cbp;
                    if (cb->owns_func && cb->ptr) {
                        delete cb->ptr->target->access.wrapped;
                        delete cb->ptr->target;
                    }

                    if (cb->owns_ptr && cb->ptr) delete cb->ptr;
                    delete cb;
                    if (cb->free_self) delete (void*)cbp;
                }
            }
            return out;
        }

        return script_object(this);
    }
};