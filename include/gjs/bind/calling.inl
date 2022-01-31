#include <gjs/util/template_utils.hpp>
#include <gjs/common/script_function.h>
#include <gjs/common/function_pointer.h>
#include <gjs/common/type_manager.h>

namespace gjs {
    template <typename... Args>
    script_object call(script_function* func, void* self, Args... args) {
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
            return script_object(func->ctx());
        }

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(func->ctx(), args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->type->signature->explicit_argc != ac) valid_call = false;

                for (u8 a = 0;a < func->type->signature->explicit_argc && valid_call;a++) {
                    script_type* tgt = func->type->signature->explicit_arg(a).tp;
                    valid_call = (tgt->id() == arg_types[a]->id());
                    if (!valid_call && tgt->requires_subtype && tgt->is_host && arg_types[a]->base_type && arg_types[a]->base_type->id() == tgt->id()) {
                        valid_call = true;
                    }
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(func->ctx());
            }

            std::vector<void*> vargs = { to_arg(args)... };
            if (self) vargs.insert(vargs.begin(), self);

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), vargs.data());
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
                return script_object(func->ctx());
            }

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), &self);
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    if ((*cbp)->keep_alive) continue;
                    raw_callback::destroy(cbp);
                }
            }
            return out;
        }

        return script_object(func->ctx());
    }

    template <typename... Args>
    script_object call(u64 moduletype_id, script_function* func, void* self, Args... args) {
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
            return script_object(func->ctx());
        }

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(func->ctx(), args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->type->signature->explicit_argc != ac) valid_call = false;

                for (u8 a = 0;a < func->type->signature->explicit_argc && valid_call;a++) {
                    script_type* tgt = func->type->signature->explicit_arg(a).tp;
                    valid_call = (tgt->id() == arg_types[a]->id());
                    if (!valid_call && tgt->requires_subtype && tgt->is_host && arg_types[a]->base_type && arg_types[a]->base_type->id() == tgt->id()) {
                        valid_call = true;
                    }
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(func->ctx());
            }

            std::vector<void*> vargs = { to_arg(moduletype_id), to_arg(args)... };
            if (self) vargs.insert(vargs.begin(), self);

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), vargs.data());
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
                return script_object(func->ctx());
            }

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }

            std::vector<void*> vargs = { to_arg(moduletype_id), to_arg(args)... };
            if (self) vargs.insert(vargs.begin(), self);

            func->ctx()->generator()->call(func, out.self(), vargs.data());
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    if ((*cbp)->keep_alive) continue;
                    raw_callback::destroy(cbp);
                }
            }
            return out;
        }

        return script_object(func->ctx());
    }

    template <typename... Args>
    script_object call(raw_callback* cb, Args... args) {
        script_function* func = cb->ptr->target;
        // validate signature
        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        bool valid_call = true;

        if constexpr (ac > 0) {
            script_type* arg_types[ac] = { arg_type(func->ctx(), args)... };
            for (u8 i = 0;i < ac;i++) {
                if (!arg_types[i]) {
                    valid_call = false;
                    break;
                }
            }

            if (valid_call) {
                if (func->type->signature->explicit_argc != ac) valid_call = false;

                for (u8 a = 0;a < func->type->signature->explicit_argc && valid_call;a++) {
                    script_type* tgt = func->type->signature->explicit_arg(a).tp;
                    valid_call = (tgt->id() == arg_types[a]->id());
                    if (!valid_call && tgt->requires_subtype && tgt->is_host && arg_types[a]->base_type && arg_types[a]->base_type->id() == tgt->id()) {
                        valid_call = true;
                    }
                }
            }

            if (!valid_call) {
                // todo exception
                return script_object(func->ctx());
            }

            std::vector<void*> vargs = { cb->ptr->data, to_arg(args)... };
            if (cb->ptr->self_obj()) vargs.insert(vargs.begin(), cb->ptr->self_obj());

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), vargs.data());
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    if ((*cbp)->keep_alive) continue;
                    raw_callback::destroy(cbp);
                }
            }
            return out;
        } else {
            if (func->type->signature->explicit_argc != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(func->ctx());
            }

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), &self);
            if constexpr (ac > 0) {
                for (u8 a = 0;a < ac;a++) {
                    if (!arg_types[a]->signature) continue;
                    // arg is a callback function that was dynamically allocated
                    raw_callback** cbp = (raw_callback**)vargs[a];
                    if ((*cbp)->keep_alive) continue;
                    raw_callback::destroy(cbp);
                }
            }
            return out;
        }

        return script_object(func->ctx());
    }

    template <typename... Args>
    script_object instantiate(script_type* type, Args... args) {
        return script_object(type->owner->context(), type, args...);
    }

    template <typename... Args>
    script_object construct_at(script_type* type, void* dest, Args... args) {
        return script_object(type->owner->context(), type, (u8*)dest, args...);
    }
};