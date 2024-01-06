#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Closure.h>
#include <tsn/bind/bind.hpp>

#include <utils/Math.hpp>

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
        b.method<void, const utils::String&>("append", &utils::String::append, access_modifier::public_access);
        b.method<void, const void*, u32>("append", &utils::String::append, access_modifier::public_access);
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

    void BindMath(Context* ctx) {
        // most vector functionality synthesized at compile time
        // to cut down on function calls
        Module* m = ctx->createHostModule("math");
        {
            auto t = bind<utils::vec2f>(m, "vec2f");
            t.prop("x", &utils::vec2f::x, public_access);
            t.prop("y", &utils::vec2f::y, public_access);
            t.method("length", &utils::vec2f::magnitude, public_access);
            t.method("lengthSq", &utils::vec2f::magnitudeSq, public_access);
            t.method("dot", &utils::vec2f::dot<f32>, public_access);
            t.method("normalize", &utils::vec2f::normalize, public_access);
            t.method("normalized", &utils::vec2f::normalized, public_access);
            t.method("toString", +[](utils::vec2f* self) {
                return utils::String::Format("%f, %f", self->x, self->y);
            }, public_access);
            t.finalize();
        }
        {
            auto t = bind<utils::vec2d>(m, "vec2d");
            t.prop("x", &utils::vec2d::x, public_access);
            t.prop("y", &utils::vec2d::y, public_access);
            t.method("length", &utils::vec2d::magnitude, public_access);
            t.method("lengthSq", &utils::vec2d::magnitudeSq, public_access);
            t.method("dot", &utils::vec2d::dot<f64>, public_access);
            t.method("normalize", &utils::vec2d::normalize, public_access);
            t.method("normalized", &utils::vec2d::normalized, public_access);
            t.method("toString", +[](utils::vec2d* self) {
                return utils::String::Format("%f, %f", self->x, self->y);
            }, public_access);
            t.finalize();
        }
        {
            auto t = bind<utils::vec3f>(m, "vec3f");
            t.prop("x", &utils::vec3f::x, public_access);
            t.prop("y", &utils::vec3f::y, public_access);
            t.prop("z", &utils::vec3f::z, public_access);
            t.method("length", &utils::vec3f::magnitude, public_access);
            t.method("lengthSq", &utils::vec3f::magnitudeSq, public_access);
            t.method("cross", &utils::vec3f::cross<f32>, public_access);
            t.method("dot", &utils::vec3f::dot<f32>, public_access);
            t.method("normalize", &utils::vec3f::normalize, public_access);
            t.method("normalized", &utils::vec3f::normalized, public_access);
            t.method("toString", +[](utils::vec3f* self) {
                return utils::String::Format("%f, %f, %f", self->x, self->y, self->z);
            }, public_access);
            t.finalize();
        }
        {
            auto t = bind<utils::vec3d>(m, "vec3d");
            t.prop("x", &utils::vec3d::x, public_access);
            t.prop("y", &utils::vec3d::y, public_access);
            t.prop("z", &utils::vec3d::z, public_access);
            t.method("length", &utils::vec3d::magnitude, public_access);
            t.method("lengthSq", &utils::vec3d::magnitudeSq, public_access);
            t.method("cross", &utils::vec3d::cross<f64>, public_access);
            t.method("dot", &utils::vec3d::dot<f64>, public_access);
            t.method("normalize", &utils::vec3d::normalize, public_access);
            t.method("normalized", &utils::vec3d::normalized, public_access);
            t.method("toString", +[](utils::vec3d* self) {
                return utils::String::Format("%f, %f, %f", self->x, self->y, self->z);
            }, public_access);
            t.finalize();
        }
        {
            auto t = bind<utils::vec4f>(m, "vec4f");
            t.prop("x", &utils::vec4f::x, public_access);
            t.prop("y", &utils::vec4f::y, public_access);
            t.prop("z", &utils::vec4f::z, public_access);
            t.prop("w", &utils::vec4f::w, public_access);
            t.method("length", &utils::vec4f::magnitude, public_access);
            t.method("lengthSq", &utils::vec4f::magnitudeSq, public_access);
            t.method<utils::vec3f, const utils::vec3f&>("cross", &utils::vec4f::cross<f32>, public_access);
            t.method<utils::vec3f, const utils::vec4f&>("cross", &utils::vec4f::cross<f32>, public_access);
            t.method("dot", &utils::vec4f::dot<f32>, public_access);
            t.method("normalize", &utils::vec4f::normalize, public_access);
            t.method("normalized", &utils::vec4f::normalized, public_access);
            t.method("toString", +[](utils::vec4f* self) {
                return utils::String::Format("%f, %f, %f, %f", self->x, self->y, self->z, self->w);
            }, public_access);
            t.finalize();
        }
        {
            auto t = bind<utils::vec4d>(m, "vec4d");
            t.prop("x", &utils::vec4d::x, public_access);
            t.prop("y", &utils::vec4d::y, public_access);
            t.prop("z", &utils::vec4d::z, public_access);
            t.prop("w", &utils::vec4d::w, public_access);
            t.method("length", &utils::vec4d::magnitude, public_access);
            t.method("lengthSq", &utils::vec4d::magnitudeSq, public_access);
            t.method<utils::vec3d, const utils::vec3d&>("cross", &utils::vec4d::cross<f64>, public_access);
            t.method<utils::vec3d, const utils::vec4d&>("cross", &utils::vec4d::cross<f64>, public_access);
            t.method("dot", &utils::vec4d::dot<f64>, public_access);
            t.method("normalize", &utils::vec4d::normalize, public_access);
            t.method("normalized", &utils::vec4d::normalized, public_access);
            t.method("toString", +[](utils::vec4d* self) {
                return utils::String::Format("%f, %f, %f, %f", self->x, self->y, self->z, self->w);
            }, public_access);
            t.finalize();
        }
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
        BindMath(ctx);

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