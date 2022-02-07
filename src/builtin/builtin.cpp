#include <gjs/builtin/builtin.h>
#include <gjs/builtin/script_pointer.h>
#include <gjs/builtin/script_array.h>
#include <gjs/builtin/script_string.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/builtin/script_math.h>
#include <gjs/builtin/script_dylib.h>
#include <gjs/builtin/script_process.h>

#include <gjs/gjs.hpp>

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
        snprintf(fmt, 7, "%%.%df", digits);
        i32 out_count = snprintf(out, 127, fmt, double(self));
        return script_string(out, out_count);
    }

    template <typename T, bool do_throw = false>
    T from_str(const script_string& str) {
        if constexpr (std::is_floating_point_v<T>) {
            const char* s = str.c_str();
            while (isspace(*s) && *s != 0) s++;

            if constexpr (sizeof(T) == sizeof(f64)) {
                static T _NaN = nan("");
                if (!*s) return _NaN;
                errno = 0;
                T val = strtod(s, nullptr);
                if (val == 0 && *s != '0' || errno != 0) return _NaN;
                return val;
            } else {
                static T _NaN = nanf("");
                if (!*s) return _NaN;
                errno = 0;
                T val = strtod(s, nullptr);
                if (val > FLT_MAX || val == 0 && *s != '0' || errno != 0) return _NaN;
                return val;
            }
        } else {
            if (str.length() == 0) {
                if constexpr (do_throw) {
                    // todo runtime exception
                }
                return T(0);
            }

            const char* s = str.c_str();
            while (isspace(*s)) s++;
            if (!*s) {
                if constexpr (do_throw) {
                    // todo runtime exception
                }

                return T(0);
            }

            if constexpr (std::is_unsigned_v<T>) {
                errno = 0;
                if (*s == '-') {
                    if constexpr (do_throw) {
                        // todo runtime exception
                    }

                    return T(0);
                }

                u64 val = strtoull(s, nullptr, 10);
                if (val > std::numeric_limits<T>::max() || val == 0 && *s != '0' || errno != 0) {
                    if constexpr (do_throw) {
                        // todo runtime exception
                    }

                    return T(0);
                }
                return val;
            } else {
                errno = 0;
                i64 val = strtoll(s, nullptr, 10);
                if (val > std::numeric_limits<T>::max() || val < std::numeric_limits<T>::min() || val == 0 && *s != '0' || errno != 0) {
                    if constexpr (do_throw) {
                        // todo runtime exception
                    }

                    return T(0);
                }
                return val;
            }
        }
    }

    template <typename T>
    bool is_nan(T self) {
        if constexpr (std::is_floating_point_v<T>) {
            return isnan<T>(self);
        } else {
            return self == std::numeric_limits<T>::max();
        }
    }

    template <typename T>
    ffi::pseudo_class<T>& bind_number_methods(ffi::pseudo_class<T>& tp) {
        // non-static
        tp.method("toFixed", to_fixed<T>);
        tp.prop("isNaN", is_nan<T>);

        // static
        tp.method("parse", from_str<T>);
        tp.method("parseThrow", from_str<T, true>);

        return tp;
    }

    void init_context(script_context* ctx) {
        set_builtin_context(ctx);
        auto str = bind<script_string>(ctx, "string");

        auto nt0 = bind<i64>(ctx, "i64");
        auto nt1 = bind<u64>(ctx, "u64");
        auto nt2 = bind<i32>(ctx, "i32");
        auto nt3 = bind<u32>(ctx, "u32");
        auto nt4 = bind<i16>(ctx, "i16");
        auto nt5 = bind<u16>(ctx, "u16");
        auto nt6 = bind<i8>(ctx, "i8");
        auto nt7 = bind<u8>(ctx, "u8");
        auto nt8 = bind<f32>(ctx, "f32");
        auto nt9 = bind<f64>(ctx, "f64");

        bind<bool>(ctx, "bool").finalize(ctx->global());

        if (typeid(char) != typeid(i8)) {
            bind<char>(ctx, "___char").finalize(ctx->global());
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

        tp = bind<subtype_t>(ctx, "subtype").finalize(ctx->global());
        tp->is_builtin = true;

        tp = bind<std::string>(ctx, "___std_str").finalize(ctx->global());
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

        auto arr = bind<script_array>(ctx, "array");
        arr.constructor<u64>();
        arr.constructor<u64, const script_array&>();
        arr.method("push", &script_array::push);
        arr.method("clear", &script_array::clear);
        arr.method("operator []", &script_array::operator[]);
        arr.method("forEach", &script_array::for_each);
        arr.method("some", &script_array::some);
        arr.method("find", &script_array::find);
        arr.method("findIndex", &script_array::findIndex);
        arr.prop("length", &script_array::length);
        tp = arr.finalize(ctx->global());
        tp->requires_subtype = true;
        tp->is_builtin = true;

        auto ptr = bind<script_pointer>(ctx, "pointer");
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

        auto buf = bind<script_buffer>(ctx, "buffer");
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

        auto fp = bind<function_pointer>(ctx, "$funcptr");
        fp.constructor<u32, u64>();
        tp = fp.finalize(ctx->global());
        tp->is_builtin = true;

        auto dylib = bind<script_dylib>(ctx, "$dylib");
        dylib.constructor<u32>();
        dylib.method("load", &script_dylib::try_load);
        dylib.method("import", &script_dylib::try_import);
        tp = dylib.finalize(ctx->global());
        tp->is_builtin = true;

        auto proc_arg = bind<process_arg>(ctx, "$proc_arg");
        proc_arg.prop("name", &process_arg::name);
        proc_arg.prop("value", &process_arg::value);
        tp = proc_arg.finalize(ctx->global());
        tp->is_builtin = true;

        auto proc = bind<script_process>(ctx, "$proc");
        proc.constructor();
        proc.prop("argc", &script_process::argc);
        proc.prop("raw_argc", &script_process::raw_argc);
        proc.method("get_arg", &script_process::get_arg);
        proc.method("get_raw_arg", &script_process::get_raw_arg);
        proc.method("exit", &script_process::exit);
        proc.method("env", &script_process::env);
        tp = proc.finalize(ctx->global());
        tp->is_builtin = true;

        bind<void*, u32, void*, u64>(ctx, raw_callback::make, "$makefunc");
        bind(ctx, script_allocate, "alloc");
        bind(ctx, script_free, "free");
        bind(ctx, script_copymem, "memcopy");
        bind(ctx, script_print, "print");

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
