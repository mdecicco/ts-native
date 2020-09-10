#include <vm/allocator.h>
#include <vm/vm_function.h>
#include <vm/context.h>
#include <vm/vm_type.h>
#include <compiler/tac.h>
#include <backends/vm.h>
#include <stdio.h>

using namespace gjs;

class foo {
    public:
        foo() : x(0), y(0), z(0), w(0.0f) {
            printf("Construct foo\n");
        }
        ~foo() {
            printf("Destruct foo\n");
        }

        i32 t(i32 a, i32* b) {
            return printf("%d, %d, %d\n", y, a, *b);
        }

        static void static_func(i32 a) {
            printf("ayyy: %d\n", a);
        }
        f32 ft(f32 a) {
            return s = a;
        }

        operator i32() { return y; }

        i32 get_x() {
            return x;
        }
        i32 set_x(i32 _x) {
            return x = _x;
        }

        i32 x;
        i32 y;
        i32 z;
        f32 w;
        static f32 s;
};
f32 foo::s = 5.5;

struct vec3 { f32 x, y, z; };
void testvec(void* vec) {
    vec3& v = *(vec3*)vec;
    printf("%f, %f, %f\n", v.x, v.y, v.z);
}

void dtestvec(void* vec) {
    vec3& v = *(vec3*)vec;
    printf("Destroyed: %f, %f, %f\n", v.x, v.y, v.z);
}

void print_foo(const foo& f) {
    printf("foo: %d, %d, %d, %f\n", f.x, f.y, f.z, f.w);
}

void print_f32(u8 i, f32 f) {
    printf("%d: %f\n", i, f);
}

void print_log(script_context& ctx) {
    for (u8 i = 0;i < ctx.compiler()->log()->errors.size();i++) {
        compile_message& m = ctx.compiler()->log()->errors[i];
        printf("%s:%d:%d: Error: %s\n", m.src.filename.c_str(), m.src.line, m.src.col, m.text.c_str());
        std::string ln = "";
        u32 wscount = 0;
        bool reachedText = false;
        for (u32 i = 0;i < m.src.line_text.length();i++) {
            if (isspace(m.src.line_text[i]) && !reachedText) wscount++;
            else {
                reachedText = true;
                ln += m.src.line_text[i];
            }
        }
        printf("%s\n", ln.c_str());
        for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
        printf("^\n");
    }

    for (u8 i = 0;i < ctx.compiler()->log()->warnings.size();i++) {
        compile_message& m = ctx.compiler()->log()->warnings[i];
        printf("%s:%d:%d: Warning: %s\n", m.src.filename.c_str(), m.src.line, m.src.col, m.text.c_str());
        std::string ln = "";
        u32 wscount = 0;
        bool reachedText = false;
        for (u32 i = 0;i < m.src.line_text.length();i++) {
            if (isspace(m.src.line_text[i]) && !reachedText) wscount++;
            else {
                reachedText = true;
                ln += m.src.line_text[i];
            }
        }
        printf("%s\n", ln.c_str());
        for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
        printf("^\n");
    }
}

void print_code(vm_backend& ctx) {
    std::string last_line = "";
    for (u32 i = 0;i < ctx.code()->size();i++) {
        vm_function* f = ctx.context()->function(i);
        if (f) {
            printf("\n[%s %s(", f->signature.return_type->name.c_str(), f->name.c_str());
            for(u8 a = 0;a < f->signature.arg_types.size();a++) {
                if (a > 0) printf(", ");
                printf("%s arg_%d -> $%s", f->signature.arg_types[a]->name.c_str(), a, register_str[u8(f->signature.arg_locs[a])]);
            }
            printf(")");

            if (f->signature.return_type->name == "void") printf(" -> null");
            else {
                if (f->signature.returns_on_stack) {
                    printf(" -> $%s (stack)", register_str[u8(f->signature.return_loc)]);
                } else {
                    printf(" -> $%s", register_str[u8(f->signature.return_loc)]);
                }
            }
            printf("]\n");
        }
        printf("0x%2.2X: %-32s", i, (*ctx.code())[i].to_string().c_str());

        auto src = ctx.map()->get(i);
        if (src.line_text != last_line) {
            printf("; %s", src.line_text.c_str());
            last_line = src.line_text;
        }
        printf("\n");
    }
}

void debug_ir_step(script_context* ctx, ir_code& code) {
    for (u32 i = 0;i < code.size();i++) {
        printf("%3.3d: %s\n", i, code[i]->to_string().c_str());
    }
}

// https://www.notion.so/3ccc9d8dba114bf8acfbbe238cb729e3?v=1a0dff3b20ba4f678bc7f5866665c4df
int main(int arg_count, const char** args) {
    std::string dir = args[0];
    dir = dir.substr(0, dir.find_last_of('\\'));
    FILE* fp = fopen((dir + "\\test.gjs").c_str(), "r");
    std::string src;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size_t sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* c = new char[sz + 1];
        memset(c, 0, sz + 1);
        fread(c, sz, 1, fp);
        src = c;
        delete [] c;
        fclose(fp);
    }

    vm_allocator* alloc = new basic_malloc_allocator();
    vm_backend gen(alloc, 4096, 4096);
    script_context ctx(&gen);
    ctx.compiler()->add_ir_step(debug_ir_step);

    try {
        auto f = ctx.bind<foo>("foo");
        f.constructor();
        f.method("t", &foo::t);
        f.method("ft", &foo::ft);
        f.method("operator i32", &foo::operator i32);
        f.method("static_func", &foo::static_func);
        f.prop("x", &foo::get_x, &foo::set_x, bind::property_flags::pf_none);
        f.prop("y", &foo::y, bind::property_flags::pf_none);
        f.prop("z", &foo::z, bind::property_flags::pf_none);
        f.prop("w", &foo::w, bind::property_flags::pf_none);
        f.prop("s", &foo::s, bind::property_flags::pf_none);
        f.finalize();

        ctx.bind(print_foo, "print_foo");
        ctx.bind(testvec, "testvec");
        ctx.bind(dtestvec, "dtestvec");
        ctx.bind(print_f32, "print_f32");
    } catch (bind_exception& e) {
        printf("%s\n", e.text.c_str());
    }

    if (!ctx.add_code("test.gjs", src)) {
        print_log(ctx);
        return 1;
    }

    print_log(ctx);
    print_code(gen);

    printf("-------------result-------------\n");
    gen.log_instructions(true);
    vm_function* func = ctx.function("it");
    if (func) ctx.call<void>(func, nullptr);
    return 0;
}