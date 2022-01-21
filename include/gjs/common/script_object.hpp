namespace gjs {
    template <typename ...Args>
    script_object::script_object(script_context* ctx, script_type* type, Args... args) {
        m_ctx = ctx;
        m_type = type;
        m_mod = nullptr;
        m_self = nullptr;
        m_owns_ptr = true;
        m_destructed = false;
        m_propInfo = nullptr;
        m_refCount = new u32(1);

        script_function* ctor = nullptr;

        script_type* thisTp = m_type;
        if (thisTp->base_type && thisTp->base_type->is_host) thisTp = thisTp->base_type;

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            std::vector<script_type*> arg_types = { arg_type(ctx, args)... };
            for (u8 a = 0;a < arg_types.size();a++) {
                if (arg_types[a]->base_type && arg_types[a]->base_type->is_host) {
                    arg_types[a] = arg_types[a]->base_type;
                }
            }

            ctor = thisTp->method("constructor", nullptr, arg_types);
        } else ctor = m_type->method("constructor", nullptr, {});

        m_self = new u8[type->size];
        if (ctor) {
            if (m_type->base_type && m_type->is_host) {
                script_type* st = m_type->sub_type;
                gjs::call(join_u32(st->owner->id(), st->id()), ctor, m_self, args...);
            }
            else gjs::call(ctor, m_self, args...);
        }
        else throw error::runtime_exception(error::ecode::r_invalid_object_constructor, type->name.c_str());
    }

    template <typename ...Args>
    script_object::script_object(script_context* ctx, script_type* type, u8* ptr, Args... args) {
        m_ctx = ctx;
        m_type = type;
        m_mod = nullptr;
        m_self = nullptr;
        m_owns_ptr = false;
        m_destructed = false;
        m_propInfo = nullptr;
        m_refCount = new u32(1);

        script_function* ctor = nullptr;

        script_type* thisTp = m_type;
        if (thisTp->base_type && thisTp->base_type->is_host) thisTp = thisTp->base_type;

        constexpr u8 ac = std::tuple_size<std::tuple<Args...>>::value;
        if constexpr (ac > 0) {
            std::vector<script_type*> arg_types = { arg_type(ctx, args)... };
            for (u8 a = 0;a < arg_types.size();a++) {
                if (arg_types[a]->base_type && arg_types[a]->base_type->is_host) {
                    arg_types[a] = arg_types[a]->base_type;
                }
            }

            ctor = thisTp->method("constructor", nullptr, arg_types);
        } else ctor = thisTp->method("constructor", nullptr, {});

        m_self = ptr;
        if (ctor) {
            if (m_type->base_type && m_type->is_host) {
                script_type* st = m_type->sub_type;
                gjs::call(join_u32(st->owner->id(), st->id()), ctor, m_self, args...);
            }
            else gjs::call(ctor, m_self, args...);
        }
        else throw error::runtime_exception(error::ecode::r_invalid_object_constructor, thisTp->name.c_str());
    }

    template <typename Ret, typename... Args>
    script_object script_object::call(const std::string& func, Args... args) {
        if (!m_self) {
            throw error::runtime_exception(error::ecode::r_call_null_obj_method, func.c_str());
            return script_object(m_ctx);
        }

        script_function* f = m_type->method(func, type_of<Ret>(m_ctx), { type_of<Args>(m_ctx)... });
        if (!f) {
            script_type* rt = type_of<Ret>(m_ctx);
            constexpr i32 argc = std::tuple_size<std::tuple<Args...>>::value;
            script_type* argTps[] = { type_of<Args>(m_ctx)..., nullptr };
            function_signature sig = function_signature(m_ctx, rt, std::is_reference_v<Ret> || std::is_pointer_v<Ret>, argTps, argc, nullptr);

            throw error::runtime_exception(error::ecode::r_invalid_object_method, m_type->name.c_str(), sig.to_string(func).c_str());
            return script_object(m_ctx);
        }

        return gjs::call(f, m_self, args...);
    }

    template <typename T>
    script_object script_object::operator = (const T& rhs) {
        using base_tp = typename remove_all<T>::type;
        if constexpr (std::is_same_v<T, script_object>) assign(rhs);
        else if constexpr (std::is_same_v<T, script_object*>) assign(*rhs);
        else {
            if (std::is_trivially_copyable_v<base_tp> && std::is_pod_v<base_tp> && m_type->size == sizeof(base_tp) && m_type->is_trivially_copyable && m_type->is_pod) {
                script_type* tp = type_of<T>(m_ctx);
                if (tp && !tp->is_primitive) {
                    // favor copy construction for structures
                    if (assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)&rhs))) return *this;
                }

                // assumes that T is an unbound type with the same layout as this object
                if constexpr (std::is_pointer_v<T>) *(base_tp*)m_self = *rhs;
                else *(base_tp*)m_self = rhs;
            } else {
                if constexpr (std::is_pointer_v<T>) {
                    script_type* tp = type_of<T>(m_ctx, true);
                    assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)rhs));
                } else {
                    script_type* tp = type_of<T>(m_ctx, true);
                    assign(script_object(m_ctx, (script_module*)nullptr, tp, (u8*)&rhs));
                }
            }
        }

        return *this;
    }

    template <typename T>
    script_object::operator T&() {
        return *(T*)m_self;
    }

    template <typename T>
    script_object::operator T*() {
        return (T*)m_self;
    }
};
