#include <gs/builtin/Builtin.h>
#include <gs/common/Context.h>
#include <gs/bind/bind.hpp>

using namespace gs::ffi;

namespace gs {
    template <typename T>
    void BindNumberType(Context* ctx, const char* name) {
        auto b = bind<T>(ctx, name);
        b.finalize(ctx->getGlobal());
    }

    void BindString(Context* ctx) {
        auto b = bind<utils::String>(ctx, "string");

        b.ctor(access_modifier::public_access);
        b.ctor<const utils::String&>(access_modifier::public_access);
        b.ctor<void*, u32>(access_modifier::private_access);
        b.dtor(access_modifier::public_access);
        
        b.finalize(ctx->getGlobal());
    }

    void AddBuiltInBindings(Context* ctx) {
        BindNumberType<i8 >(ctx, "i8" );
        BindNumberType<i16>(ctx, "i16");
        BindNumberType<i32>(ctx, "i32");
        BindNumberType<i64>(ctx, "i64");
        BindNumberType<u8 >(ctx, "u8" );
        BindNumberType<u16>(ctx, "u16");
        BindNumberType<u32>(ctx, "u32");
        BindNumberType<u64>(ctx, "u64");
        BindNumberType<f32>(ctx, "f32");
        BindNumberType<f64>(ctx, "f64");

        auto v  = bind<void>(ctx, "void").finalize(ctx->getGlobal());
        auto vp = bind<void*>(ctx, "data").finalize(ctx->getGlobal());
        auto b = bind<bool>(ctx, "boolean").finalize(ctx->getGlobal());
        auto p = bind<poison_t>(ctx, "$poison").finalize(ctx->getGlobal());
        auto ectx = bind<ExecutionContext>(ctx, "$exec").dtor(gs::private_access).finalize(ctx->getGlobal());

        BindString(ctx);
    }
};