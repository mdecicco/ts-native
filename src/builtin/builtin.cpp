#include <common/script_function.h>
#include <common/script_type.h>
#include <builtin/builtin.h>
#include <builtin/script_array.h>
#include <builtin/script_string.h>
#include <builtin/script_buffer.h>
#include <common/context.h>
#include <bind/bind.h>

namespace gjs {
    // todo: thread_id:ctx map
    static script_context* ctx = nullptr;

    void set_builtin_context(script_context* _ctx) {
        ctx = _ctx;
    }

    script_context* current_ctx() {
        return ctx;
    }
    u32 script_print(const std::string& str);
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

        if (typeid(char) != typeid(i8)) {
            tp = nullptr;
            tp = ctx->types()->add("___char", typeid(char).name());
            tp->is_host = true;
            tp->is_primitive = true;
            tp->is_builtin = true;
            tp->size = sizeof(char);
        }

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

        tp = ctx->bind<subtype_t>("subtype").finalize();
        tp->is_builtin = true;

        auto std_str = ctx->bind<std::string>("___std_str").finalize();
        std_str->size = sizeof(std::string);

        auto str = ctx->bind<script_string>("string");
        str.constructor();
        str.constructor<void*, u32>();
        str.constructor<const script_string&>();
        str.method("operator []", &script_string::operator[]);
        str.method("operator =", METHOD_PTR(script_string, operator=, script_string&, const script_string&));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, const script_string&));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, const script_string&));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, char));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, char));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, i64));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, i64));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, u64));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, u64));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, f32));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, f32));
        str.method("operator +=", METHOD_PTR(script_string, operator+=, script_string&, f64));
        str.method("operator +", METHOD_PTR(script_string, operator+, script_string, f64));
        str.method("operator data", &script_string::operator void *);
        str.method("operator ___std_str", &script_string::operator std::string);
        str.prop("length", &script_string::length);
        tp = str.finalize();
        tp->is_builtin = true;

        auto arr = ctx->bind<script_array>("array");
        arr.constructor<script_type*>();
        arr.method("push", &script_array::push);
        arr.method("operator []", &script_array::operator[]);
        arr.prop("length", &script_array::length);
        tp = arr.finalize();
        tp->requires_subtype = true;
        tp->is_builtin = true;

        auto buf = ctx->bind<script_buffer>("buffer");
        buf.constructor();
        buf.constructor<u64>();
        buf.method("write", METHOD_PTR(script_buffer, write, void, void*, u64));
        buf.method("read", METHOD_PTR(script_buffer, read, void, void*, u64));
        buf.method("write", METHOD_PTR(script_buffer, write, void, script_buffer*, u64));
        buf.method("read", METHOD_PTR(script_buffer, read, void, script_buffer*, u64));
        buf.method("write", METHOD_PTR(script_buffer, write, void, script_string*));
        buf.method("read", METHOD_PTR(script_buffer, read, void, script_string*));
        buf.method("read", METHOD_PTR(script_buffer, read, void, script_string*, u32));
        buf.method("data", METHOD_PTR(script_buffer, data, void*, u64));
        buf.method("data", METHOD_PTR(script_buffer, data, void*));

        #define rm(tp) buf.method("read", &script_buffer::read<tp>);
        buf.method("read", &script_buffer::read<u8>);
        buf.method("read", &script_buffer::read<i8>);
        buf.method("read", &script_buffer::read<u16>);
        buf.method("read", &script_buffer::read<i16>);
        buf.method("read", &script_buffer::read<u32>);
        buf.method("read", &script_buffer::read<i32>);
        buf.method("read", &script_buffer::read<u64>);
        buf.method("read", &script_buffer::read<i64>);
        buf.method("read", &script_buffer::read<f32>);
        buf.method("read", &script_buffer::read<f64>);

        buf.prop("resizes", &script_buffer::can_resize);
        buf.prop("size", &script_buffer::size);
        buf.prop("remaining", &script_buffer::remaining);
        buf.prop("capacity", &script_buffer::capacity);
        buf.prop("position", CONST_METHOD_PTR(script_buffer, position, u64), METHOD_PTR(script_buffer, position, u64, u64));
        buf.prop("at_end", &script_buffer::at_end);
        tp = buf.finalize();
        tp->is_builtin = true;


        ctx->bind(script_allocate, "alloc");
        ctx->bind(script_free, "free");
        ctx->bind(script_copymem, "memcopy");
        ctx->bind(script_print, "print");
    }

    u32 script_print(const std::string& str) {
        return printf(str.c_str());
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