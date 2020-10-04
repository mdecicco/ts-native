#include <common/script_function.h>
#include <common/script_type.h>
#include <builtin/builtin.h>
#include <builtin/script_array.h>
#include <builtin/script_string.h>
#include <common/context.h>
#include <bind/bind.h>

namespace gjs {
    // todo: thread_id:ctx map
    static script_context* ctx = nullptr;

    void set_builtin_context(script_context* _ctx) {
        ctx = _ctx;
    }

    void init_context(script_context* ctx) {
        script_type* tp = nullptr;

        tp = nullptr;
        tp = ctx->types()->add("i64", typeid(i64).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_builtin = true;
        tp->size = sizeof(i64);

        tp = nullptr;
        tp = ctx->types()->add("u64", typeid(u64).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_unsigned = true;
        tp->is_builtin = true;
        tp->size = sizeof(u64);

        tp = nullptr;
        tp = ctx->types()->add("i32", typeid(i32).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_builtin = true;
        tp->size = sizeof(i32);

        tp = nullptr;
        tp = ctx->types()->add("u32", typeid(u32).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_unsigned = true;
        tp->is_builtin = true;
        tp->size = sizeof(u32);

        tp = nullptr;
        tp = ctx->types()->add("i16", typeid(i16).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_builtin = true;
        tp->size = sizeof(i16);

        tp = nullptr;
        tp = ctx->types()->add("u16", typeid(u16).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_unsigned = true;
        tp->is_builtin = true;
        tp->size = sizeof(u16);

        tp = nullptr;
        tp = ctx->types()->add("i8", typeid(i8).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_builtin = true;
        tp->size = sizeof(i8);

        tp = nullptr;
        tp = ctx->types()->add("u8", typeid(u8).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_unsigned = true;
        tp->is_builtin = true;
        tp->size = sizeof(u8);

        tp = ctx->types()->add("f32", typeid(f32).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_floating_point = true;
        tp->is_builtin = true;
        tp->size = sizeof(f32);

        tp = ctx->types()->add("f64", typeid(f64).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_floating_point = true;
        tp->is_builtin = true;
        tp->size = sizeof(f64);

        tp = ctx->types()->add("bool", typeid(bool).name());
        tp->is_host = true;
        tp->is_primitive = true;
        tp->is_builtin = true;
        tp->size = sizeof(bool);

        tp = ctx->types()->add("void", "void");
        tp->is_host = true;
        tp->is_builtin = true;
        tp->size = 0;

        tp = ctx->types()->add("error_type", "error_type");
        tp->is_host = true;
        tp->is_builtin = true;
        tp->size = 0;

        tp = ctx->types()->add("data", "void*");
        tp->is_host = true;
        tp->is_builtin = true;
        tp->size = sizeof(void*);
        tp->is_unsigned = true;

        ctx->bind<subtype_t>("subtype").finalize();

        auto str = ctx->bind<script_string>("string");
        str.constructor();
        str.constructor<const script_string&>();
        str.method("operator []", &script_string::operator[]);
        str.method("operator =", METHOD_PTR(script_string, operator=, script_string&, const script_string&));
        str.prop("length", &script_string::length);

        auto arr = ctx->bind<script_array>("array");
        arr.constructor<script_type*>();
        arr.method("push", &script_array::push);
        arr.method("operator []", &script_array::operator[]);
        arr.prop("length", &script_array::length);
        tp = arr.finalize();
        tp->requires_subtype = true;
        tp->is_builtin = true;


        ctx->bind(script_allocate, "alloc");
        ctx->bind(script_free, "free");
        ctx->bind(script_copymem, "memcopy");
    }

    void* script_allocate(u64 size) {
        // todo: allocate from context memory
        void* mem = malloc(size);
        return mem;
    }

    void script_free(void* ptr) {
        free(ptr);
    }

    void script_copymem(void* to, void* from, u64 size) {
        memmove(to, from, size);
    }
};