#include <gjs/util/template_utils.h>
#include <gjs/common/script_function.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/exec_context.h>

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

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx, to_arg(args)... };
            if (self) vargs.insert(vargs.begin() + 1, self);

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }

            func->ctx()->generator()->call(func, out.self(), vargs.data());

            static constexpr bool arg_is_wrapped_cb[] = { is_callback_v<Args>... };
            for (u8 a = 0;a < ac;a++) {
                if (!arg_types[a]->signature) continue;
                
                if (!arg_is_wrapped_cb[a]) {
                    // Argument is a callback function that was dynamically allocated as a result of to_arg
                    // 
                    // If the application passes a raw function pointer or lambda function as an argument, it will be wrapped inside
                    // a dynamically allocated raw_callback via the call to 'to_arg' that's used to construct vargs here.
                    // It must be destroyed.
                    // 
                    // If the user passes a pre-constructed callback that's wrapped with the callback<...> template class, then it's
                    // not this code's responsibility to destroy it. (At least not right now, this call could be nested below a call
                    // where it _was_ allocated dynamically via 'to_arg')
                    raw_callback::destroy((raw_callback**)vargs[a]);
                }
            }

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
            }

            delete ectx;

            return out;
        } else {
            if (func->type->signature->explicit_argc != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(func->ctx());
            }

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx };
            if (self) vargs.insert(vargs.begin() + 1, self);

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }

            func->ctx()->generator()->call(func, out.self(), vargs.data());

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
            }

            delete ectx;
            
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

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx, to_arg(moduletype_id), to_arg(args)... };
            if (self) vargs.insert(vargs.begin() + 1, self);

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), vargs.data());

            static constexpr bool arg_is_wrapped_cb[] = { is_callback_v<Args>... };
            for (u8 a = 0;a < ac;a++) {
                if (!arg_types[a]->signature) continue;

                if (!arg_is_wrapped_cb[a]) {
                    // Argument is a callback function that was dynamically allocated as a result of to_arg
                    // 
                    // If the application passes a raw function pointer or lambda function as an argument, it will be wrapped inside
                    // a dynamically allocated raw_callback via the call to 'to_arg' that's used to construct vargs here.
                    // It must be destroyed.
                    // 
                    // If the user passes a pre-constructed callback that's wrapped with the callback<...> template class, then it's
                    // not this code's responsibility to destroy it. (At least not right now, this call could be nested below a call
                    // where it _was_ allocated dynamically via 'to_arg')
                    raw_callback::destroy((raw_callback**)vargs[a]);
                }
            }

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
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

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx, to_arg(moduletype_id), to_arg(args)... };
            if (self) vargs.insert(vargs.begin() + 1, self);

            func->ctx()->generator()->call(func, out.self(), vargs.data());

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
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

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx, cb->ptr->data, to_arg(args)... };
            if (cb->ptr->self_obj()) vargs.insert(vargs.begin() + 2, cb->ptr->self_obj());

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }
            func->ctx()->generator()->call(func, out.self(), vargs.data());

            static constexpr bool arg_is_wrapped_cb[] = { is_callback_v<Args>... };
            for (u8 a = 0;a < ac;a++) {
                if (!arg_types[a]->signature) continue;

                if (!arg_is_wrapped_cb[a]) {
                    // Argument is a callback function that was dynamically allocated as a result of to_arg
                    // 
                    // If the application passes a raw function pointer or lambda function as an argument, it will be wrapped inside
                    // a dynamically allocated raw_callback via the call to 'to_arg' that's used to construct vargs here.
                    // It must be destroyed.
                    // 
                    // If the user passes a pre-constructed callback that's wrapped with the callback<...> template class, then it's
                    // not this code's responsibility to destroy it. (At least not right now, this call could be nested below a call
                    // where it _was_ allocated dynamically via 'to_arg')
                    raw_callback::destroy((raw_callback**)vargs[a]);
                }
            }

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
            }

            return out;
        } else {
            if (func->type->signature->explicit_argc != 0) valid_call = false;

            if (!valid_call) {
                // exception
                return script_object(func->ctx());
            }

            exec_context *ectx = new exec_context();
            std::vector<void*> vargs = { (void*)ectx, cb->ptr->data };
            if (cb->ptr->self_obj()) vargs.insert(vargs.begin() + 2, cb->ptr->self_obj());

            script_object out = script_object(func->ctx(), (script_module*)nullptr, func->type->signature->return_type, nullptr);
            if (func->type->signature->return_type->size > 0) {
                out.set_self(new u8[func->type->signature->return_type->size]); 
            }

            func->ctx()->generator()->call(func, out.self(), vargs.data());

            if (ectx->trace.has_error) {
                script_string err = ectx->trace.error;
                delete ectx;
                throw error::runtime_exception(error::ecode::r_any_error, err.c_str());
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
