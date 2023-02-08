#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/bind/bind.hpp>

using namespace tsn::ffi;

namespace tsn {
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

        auto vp = bind<void*>(ctx, "data");
        type_meta& vpi = vp.info();
        vpi.is_primitive = 1;
        vpi.is_unsigned = 1;
        vpi.is_integral = 1;
        vp.access(trusted_access).finalize(ctx->getGlobal());

        auto n = bind<null_t>(ctx, "null_t");
        type_meta& ni = n.info();
        ni.is_primitive = 1;
        ni.is_integral = 1;
        ni.is_unsigned = 1;
        n.finalize(ctx->getGlobal());

        auto b = bind<bool>(ctx, "boolean").finalize(ctx->getGlobal());
        auto p = bind<poison_t>(ctx, "$poison").finalize(ctx->getGlobal());
        auto ectx = bind<ExecutionContext>(ctx, "$exec").dtor(tsn::private_access).finalize(ctx->getGlobal());

        BindString(ctx);
        BindMemory(ctx);
    }
};