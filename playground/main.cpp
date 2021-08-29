#include <gjs/gjs.h>
#include <stdio.h>
// #include <gjs/backends/b_win_x86_64.h>
#include <gjs/builtin/script_math.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_vec4.h>

#include <type_traits>

using namespace gjs;
using namespace gjs::math;

void remove_unused_regs_pass (script_context* ctx, compilation_output& in) {
    u64 csz = in.code.size();
    for (u64 c = 0;c < in.code.size() || c == u64(-1);c++) {
        if (compile::is_assignment(in.code[c]) && in.code[c].op != compile::operation::call) {
            u32 assigned_reg = in.code[c].operands[0].reg_id();

            bool used = false;
            for (u64 c1 = c + 1;c1 < in.code.size() && !used;c1++) {
                auto& i = in.code[c1];
                for (u8 o = 0;o < 3 && !used;o++) {
                    used = i.operands[o].valid() && !i.operands[o].is_imm() && i.operands[o].reg_id() == assigned_reg;
                }
            }

            if (!used) {
                in.erase(c);
                c--;
            }
        }
    }

    if (csz > in.code.size()) remove_unused_regs_pass(ctx, in);
}

void tttt(i32 a, i8 b, f32 c) {
    printf("%d, '%c', %f\n", a, b, c);
}

typedef void (*t)(i32, i8, f32);
void func (callback<t> arg) {
    arg(1, 'a', 1.0f);
}

int main(int arg_count, const char** args) {
    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    int a = 69;
    char b = 61;
    float c = 3.14f;

    auto test = [&](int a1, char b1, float c1) {
        a = a1;
        b = b1;
        c = c1;
    };
    //script_function* ttt = ctx.bind(tttt, "ttt");
    //function_pointer fptr(ttt);
    //func(&fptr);

    //u64 sz0 = sizeof(callback<t>);
    //u64 sz1 = sizeof(raw_callback);

    //raw_callback* rcb = raw_callback::make(&fptr);
    //func(*((callback<t>*)&rcb));

    script_function* test_tttt = ctx.bind(tttt, "test_tttt");
    script_function* test_cb = ctx.bind(func, "test_cb");


    be.commit_bindings();
    //be.log_ir(true);
    ctx.io()->set_cwd_from_args(arg_count, args);
    //ctx.compiler()->add_ir_step(remove_unused_regs_pass);
    //ctx.compiler()->add_ir_step(debug_ir_step);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    print_code(&be);

    function_pointer tptr(mod->function("tttt"));
    auto cb = callback<t>(&tptr);
    cb(1, 'a', 3.0f);

    // be.log_instructions(true);

    mod->init();

    struct f0 { i32 a, b, c; };
    struct f1 { f0 a, b, c; };

    mod->local("y")["a"]["a"] = 55;

    f1 t = mod->function("func")->call(nullptr);

    script_object obj = ctx.instantiate(mod->type("t"), 5);
    obj.call("print");

    struct {
        f32 x, y;
    } vec = mod->function("vec")->call(nullptr);

    struct {
        f32 x, y;
    } vec1 = mod->function("vec1")->call(nullptr);

    struct {
        f32 x, y, z, w;
    } v4 = mod->function("v4")->call(nullptr);
    return 0;
}