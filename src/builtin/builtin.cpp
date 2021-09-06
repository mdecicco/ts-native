#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/builtin/builtin.h>
#include <gjs/builtin/script_pointer.h>
#include <gjs/builtin/script_array.h>
#include <gjs/builtin/script_string.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/common/function_pointer.h>
#include <gjs/bind/bind.h>
#include <gjs/builtin/script_math.h>

namespace gjs {
    // todo: thread_id:ctx map
    static script_context* ctx = nullptr;

    void set_builtin_context(script_context* _ctx) {
        ctx = _ctx;
    }

    script_context* current_ctx() {
        return ctx;
    }
    u32 script_print(const script_string& str);

    template <typename T>
    script_string to_fixed(T self, u8 digits) {
        char fmt[8] = { 0 };
        char out[128] = { 0 };
        snprintf(fmt, 7, "\%%.%df", digits);
        i32 out_count = snprintf(out, 127, fmt, double(self));
        return script_string(out, out_count);
    }

    template <typename T>
    bind::pseudo_class<T>& bind_number_methods(bind::pseudo_class<T>& tp) {
        tp.method("toFixed", to_fixed<T>);
        return tp;
    }

    void init_context(script_context* ctx) {
        set_builtin_context(ctx);
        auto str = ctx->bind<script_string>("string");

        auto nt0 = ctx->bind<i64>("i64");
        auto nt1 = ctx->bind<u64>("u64");
        auto nt2 = ctx->bind<i32>("i32");
        auto nt3 = ctx->bind<u32>("u32");
        auto nt4 = ctx->bind<i16>("i16");
        auto nt5 = ctx->bind<u16>("u16");
        auto nt6 = ctx->bind<i8>("i8");
        auto nt7 = ctx->bind<u8>("u8");
        auto nt8 = ctx->bind<f32>("f32");
        auto nt9 = ctx->bind<f64>("f64");

        ctx->bind<bool>("bool").finalize(ctx->global());

        if (typeid(char) != typeid(i8)) {
            ctx->bind<char>("___char").finalize(ctx->global());
        }

        script_type* tp = nullptr;
        tp = ctx->global()->types()->add("void", typeid(void).name());
        tp->owner = ctx->global();
        tp->is_host = true;
        tp->is_builtin = true;

        tp = ctx->global()->types()->add("data", typeid(void*).name());
        tp->owner = ctx->global();
        tp->is_host = true;
        tp->is_builtin = true;
        tp->size = sizeof(void*);
        tp->is_unsigned = true;
        tp->is_pod = true;

        tp = ctx->global()->types()->add("error_type", "error_type");
        tp->owner = ctx->global();
        tp->is_host = true;
        tp->is_builtin = true;
        tp->size = 0;

        tp = ctx->bind<subtype_t>("subtype").finalize(ctx->global());
        tp->is_builtin = true;

        tp = ctx->bind<std::string>("___std_str").finalize(ctx->global());
        tp->size = sizeof(std::string);

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
        tp = str.finalize(ctx->global());
        tp->is_builtin = true;

        auto arr = ctx->bind<script_array>("array");
        arr.constructor<u64>();
        arr.method("push", &script_array::push);
        arr.method("operator []", &script_array::operator[]);
        arr.prop("length", &script_array::length);
        tp = arr.finalize(ctx->global());
        tp->requires_subtype = true;
        tp->is_builtin = true;

        auto ptr = ctx->bind<script_pointer>("pointer");
        ptr.constructor<u64>();
        ptr.constructor<u64, script_pointer&>();
        ptr.method("reset", &script_pointer::reset);
        ptr.method("share", &script_pointer::share);
        ptr.method("release", &script_pointer::release);
        ptr.method("operator =", METHOD_PTR(script_pointer, operator=, script_pointer&, const script_pointer&));
        ptr.method("operator =", METHOD_PTR(script_pointer, operator=, script_pointer&, subtype_t*));
        ptr.method("operator subtype", &script_pointer::operator gjs::subtype_t *);
        ptr.prop("is_null", &script_pointer::is_null);
        ptr.prop("reference_count", &script_pointer::references);
        ptr.prop("value", &script_pointer::get);
        tp = ptr.finalize(ctx->global());
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
        tp = buf.finalize(ctx->global());
        tp->is_builtin = true;

        auto fp = ctx->bind<function_pointer>("$funcptr");
        fp.constructor<u32, u64>();
        tp = fp.finalize(ctx->global());
        tp->is_builtin = true;

        ctx->bind<void*, u32, void*, u64>(raw_callback::make, "$makefunc");
        ctx->bind(script_allocate, "alloc");
        ctx->bind(script_free, "free");
        ctx->bind(script_copymem, "memcopy");
        ctx->bind(script_print, "print");

        bind_number_methods(nt0).finalize(ctx->global());
        bind_number_methods(nt1).finalize(ctx->global());
        bind_number_methods(nt2).finalize(ctx->global());
        bind_number_methods(nt3).finalize(ctx->global());
        bind_number_methods(nt4).finalize(ctx->global());
        bind_number_methods(nt5).finalize(ctx->global());
        bind_number_methods(nt6).finalize(ctx->global());
        bind_number_methods(nt7).finalize(ctx->global());
        bind_number_methods(nt8).finalize(ctx->global());
        bind_number_methods(nt9).finalize(ctx->global());

        math::bind_math(ctx);
    }

    u32 script_print(const script_string& str) {
        return printf("%s\n", str.c_str());
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