#pragma once
#include <gjs/util/template_utils.h>
#include <gjs/util/callback.h>
#include <gjs/common/types.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_type.h>

#include <string>
#include <vector>
#include <gjs/util/robin_hood.h>

namespace gjs {
    namespace ffi {
        struct wrapped_class;
    };

    class script_context;
    class script_module;
    class function_signature;

    class type_manager {
        public:
            type_manager(script_context* ctx);
            ~type_manager();

            void merge(type_manager* new_types);

            script_type* get(const std::string& internal_name);

            script_type* get(u32 id);

            script_type* get(const function_signature& sig, const std::string& name = "", bool is_dtor = false);

            template <typename T>
            script_type* get(bool do_throw = false) {
                if constexpr (std::is_same_v<T, void*>) {
                    script_type* tp = get(typeid(void*).name());
                    if (do_throw && !tp) throw error::runtime_exception(error::ecode::r_unbound_type, base_type_name<T>());
                    return tp;
                }

                if constexpr (is_callback<T>::value) {
                    return T::type(m_ctx);
                }

                if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
                    using FT = FunctionTraits<T>;
                    script_type* rt = get<FT::RetType>();
                    if (!rt) {
                        if (do_throw) throw error::runtime_exception(error::ecode::r_unbound_type, base_type_name<FT::RetType>());
                        return nullptr;
                    }

                    constexpr i32 argc = FT::ArgCount;
                    script_type* argTps[FT::ArgCount];
                    FT::arg_types(m_ctx->types(), argTps, do_throw);

                    for (u8 i = 0;i < FT::ArgCount;i++) {
                        if (!argTps[i]) return nullptr;
                    }

                    return get(function_signature(
                        m_ctx,
                        rt,
                        std::is_reference_v<FT::RetType> || std::is_pointer_v<FT::RetType>,
                        argTps,
                        FT::ArgCount,
                        nullptr
                    ));
                }

                script_type* tp = get(base_type_name<T>());
                if (do_throw && !tp) throw error::runtime_exception(error::ecode::r_unbound_type, typeid(remove_all<T>).name());
                return tp;
            }

            script_type* add(const std::string& name, const std::string& internal_name);

            script_type* finalize_class(ffi::wrapped_class* wrapped, script_module* mod);

            std::vector<script_type*> all();

            script_context* ctx();

        protected:
            void add(script_type* tp);
            friend class script_function;
            friend class script_context;
            script_context* m_ctx;
            robin_hood::unordered_map<std::string, script_type*> m_types;
            robin_hood::unordered_map<u32, script_type*> m_types_by_id;
    };
};
