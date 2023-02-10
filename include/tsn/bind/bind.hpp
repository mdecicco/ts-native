#pragma once
#include <tsn/bind/bind.h>
#include <tsn/bind/bind_type.hpp>
#include <tsn/bind/bind_func.hpp>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>

namespace tsn {
    namespace ffi {
        template <typename Cls>
        std::enable_if_t<std::is_fundamental_v<typename remove_all<Cls>::type>, PrimitiveTypeBinder<Cls>>
        bind(Context* ctx, const utils::String& name) {
            return PrimitiveTypeBinder<Cls>(ctx->getFunctions(), ctx->getTypes(), name, "::" + name);
        }

        template <typename Cls>
        std::enable_if_t<!std::is_fundamental_v<typename remove_all<Cls>::type>, ObjectTypeBinder<Cls>>
        bind(Context* ctx, const utils::String& name) {
            return ObjectTypeBinder<Cls>(ctx->getFunctions(), ctx->getTypes(), name, "::" + name);
        }

        template <typename Ret, typename... Args>
        Function* bind(Context* ctx, const utils::String& name, Ret (*func)(Args...), access_modifier access) {
            return bind_function(ctx->getFunctions(), ctx->getTypes(), name, func, access);
        }

        template <typename Ret, typename... Args>
        Function* bind(Module* mod, const utils::String& name, Ret (*func)(Args...), access_modifier access) {
            Function* fn = bind_function(mod->getContext()->getFunctions(), mod->getContext()->getTypes(), name, func, access);
            if (fn) mod->addFunction(fn);
            return fn;
        }
    };
};