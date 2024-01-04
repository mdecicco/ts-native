#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Closure.h>
#include <tsn/bind/bind.hpp>

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
        return printf(str.c_str());
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
        b.prop("length", &utils::String::size, public_access);
        
        b.finalize();
    }

    void BindPointer(Context* ctx) {
        Module* m = ctx->getModule("trusted/pointer");
        if (!m) return;

        m->init();

        TemplateType* ptr = (TemplateType*)m->allTypes().find([](const DataType* t) { return t->getName() == "Pointer"; });
        if (!ptr) return;

        ctx->getGlobal()->addForeignType(ptr);
        ctx->getTypes()->addForeignType(ptr);
        
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt8() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt8() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt16() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt16() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getFloat32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getFloat64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getVoidPtr() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getString() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getBoolean() });
    }

    void BindArray(Context* ctx) {
        Module* m = ctx->getModule("trusted/array");
        if (!m) return;

        m->init();

        TemplateType* arr = (TemplateType*)m->allTypes().find([](const DataType* t) { return t->getName() == "Array"; });
        if (!arr) return;
        
        ctx->getGlobal()->addForeignType(arr);
        ctx->getTypes()->addForeignType(arr);
        
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getInt8() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getUInt8() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getInt16() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getUInt16() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getInt32() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getUInt32() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getInt64() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getUInt64() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getFloat32() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getFloat64() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getVoidPtr() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getString() });
        ctx->getPipeline()->specializeTemplate(arr, { ctx->getTypes()->getBoolean() });
    }

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

        bind(ctx, "print", print);

        // pointer compilation needs the above
        ctx->getTypes()->updateCachedTypes();

        BindPointer(ctx);

        // array compilation needs pointer
        ctx->getTypes()->updateCachedTypes();

        BindArray(ctx);

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