#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/bind/bind.hpp>
#include <tsn/compiler/Logger.h>

using namespace tsn::ffi;

namespace tsn {
    template <typename T>
    void BindNumberType(Context* ctx, const char* name) {
        auto b = bind<T>(ctx, name);
        b.finalize();
    }

    template <typename T>
    void ExtendNumberType(Context* ctx) {
        auto tp = extend<T>(ctx);

        tp.method("toString", +[](T* self) {
            if constexpr (std::is_floating_point_v<T>) {
                return utils::String::Format("%f", *self);
            } else if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_unsigned_v<T>) {
                    return utils::String::Format("%llu", (u64)*self);
                } else {
                    return utils::String::Format("%lli", (i64)*self);
                }
            }
        }, public_access);
    }

    i32 print(const utils::String& str) {
        return printf("%s", str.c_str());
    }

    void BindString(Context* ctx) {
        auto b = bind<utils::String>(ctx, "string");

        b.ctor(access_modifier::public_access);
        b.ctor<const utils::String&>(access_modifier::public_access);
        b.ctor<void*, u32>(access_modifier::trusted_access);
        b.dtor(access_modifier::public_access);
        b.method<utils::String , const utils::String&>("operator +"  , &utils::String::operator+   , access_modifier::public_access);
        b.method<utils::String&, const utils::String&>("operator +=" , &utils::String::operator+=  , access_modifier::public_access);
        b.method<utils::String&, const utils::String&>("operator ="  , &utils::String::operator=   , access_modifier::public_access);
        b.method<utils::String&, const utils::String&>("operator ="  , &utils::String::operator=   , access_modifier::public_access);
        b.method<bool          , const utils::String&>("operator ==" , &utils::String::operator==  , access_modifier::public_access);
        b.method<i64           , const utils::String&>("firstIndexOf", &utils::String::firstIndexOf, access_modifier::public_access);
        b.method<i64           , const utils::String&>("lastIndexOf" , &utils::String::lastIndexOf , access_modifier::public_access);
        b.method<void          , const utils::String&, const utils::String&>("replace", &utils::String::replaceAll, access_modifier::public_access);
        b.method("trim", &utils::String::trim, access_modifier::public_access);
        b.method("clone", &utils::String::clone, access_modifier::public_access);
        b.method("toUpperCase", &utils::String::toUpperCase, access_modifier::public_access);
        b.method("toLowerCase", &utils::String::toLowerCase, access_modifier::public_access);
        b.method<void, const utils::String&>("append", &utils::String::append, access_modifier::public_access);
        b.method<void, const void*, u32>("append", &utils::String::append, access_modifier::public_access);
        b.prop("length", &utils::String::size, public_access);
        
        b.finalize();
    }

    void BindPointer(Context* ctx);
    void BindArray(Context* ctx);
    void BindMath(Context* ctx);

    void ExtendString(Context* ctx) {
        auto b = extend<utils::String>(ctx);
        b.method<utils::Array<u32>, const utils::String&>("indicesOf", &utils::String::indicesOf, access_modifier::public_access);
        b.method<utils::Array<utils::String>, const utils::String&>("split", &utils::String::split, access_modifier::public_access);
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

        auto v  = bind<void>(ctx, "void").finalize();

        auto vp = bind<void*>(ctx, "data");
        type_meta& vpi = vp.info();
        vpi.is_primitive = 1;
        vpi.is_unsigned = 1;
        vpi.is_integral = 1;
        vp.access(trusted_access).finalize();

        auto n = bind<null_t>(ctx, "null_t").finalize();

        auto b = bind<bool>(ctx, "boolean").finalize();
        auto p = bind<poison_t>(ctx, "$poison").finalize();
        auto ectx = bind<ExecutionContext>(ctx, "$exec").dtor(tsn::private_access).finalize();
        
        auto cb = bind<CaptureData>(ctx, "$capture_data");
        cb.ctor<const CaptureData&>(public_access);
        cb.dtor(private_access);
        cb.finalize();

        auto cbr = bind<Closure>(ctx, "$closure");
        cbr.ctor<const Closure&>(public_access);
        cbr.ctor<CaptureData*>(public_access);
        cbr.dtor(public_access);
        cbr.finalize();

        BindString(ctx);
        BindMemory(ctx);
        BindMath(ctx);

        bind(ctx, "print", print);

        ctx->getTypes()->updateCachedTypes();

        BindPointer(ctx);
        ctx->getTypes()->updateCachedTypes();

        BindArray(ctx);
        ctx->getTypes()->updateCachedTypes();

        auto ev = extend<void*>(ctx);
        ev.method("toString", +[](void** self) {
            return utils::String::Format("0x%X", reinterpret_cast<u64>(*self));
        }, public_access);

        ExtendNumberType<i8 >(ctx);
        ExtendNumberType<i16>(ctx);
        ExtendNumberType<i32>(ctx);
        ExtendNumberType<i64>(ctx);
        ExtendNumberType<u8 >(ctx);
        ExtendNumberType<u16>(ctx);
        ExtendNumberType<u32>(ctx);
        ExtendNumberType<u64>(ctx);
        ExtendNumberType<f32>(ctx);
        ExtendNumberType<f64>(ctx);

        ExtendString(ctx);
    }
};