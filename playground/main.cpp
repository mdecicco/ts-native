#include <gjs.h>
#include <stdio.h>
#include <backends/b_win_x86_64.h>

using namespace gjs;

void print_f32(u16 i, f32 f) {
    printf("%d: %f\n", i, f);
}

class test {
    public:
        test() { x = 0.0f; }

        f32 get_x() { return x; }
        f32 set_x(f32 _x) { return x = _x; }

        f32 x;
};

void print_i32(i32 i) {
    printf("%d\n", i);
}

int test_llvm(int argc, char *argv[]) {
    win64_backend be(argc, argv);
    script_context ctx(&be);
    be.commit_bindings();

    be.log_ir(true);
    ctx.compiler()->add_ir_step(debug_ir_step);

    script_module* mod = ctx.add_code("x86_test",
        "i32 t(i32 y) {\n"
        "   i32 a;\n"
        "   i32 s = y;\n"
        "   for (a = 0;a <= 10;a++) {\n"
        "       s += a;\n"
        "   }\n"
        "   print(s.toFixed(0));\n"
        "   if (s > 5) a = 32;\n"
        "   s = a;\n"
        "   return s;\n"
        "}\n"
    );

    if (!mod) {
        print_log(&ctx);
        return -1;
    }

    mod->init();

    script_function* f = mod->function("t");
    i32 b = 0;
    ctx.call(f, &b, -50);

    return 0;
}

// https://www.notion.so/3ccc9d8dba114bf8acfbbe238cb729e3?v=1a0dff3b20ba4f678bc7f5866665c4df
int main(int arg_count, const char** args) {
    test_llvm(arg_count, const_cast<char**>(args));

    std::string dir = "";
    auto dp = split(args[0], "/\\");
    for (u16 i = 0;i < dp.size() - 1;i++) dir += dp[i] + "/";

    win64_backend be(arg_count, const_cast<char**>(args));
    script_context ctx(&be);
    ctx.io()->set_cwd(dir);

    // ctx.compiler()->add_ir_step(debug_ir_step);

    ctx.bind(print_f32, "print_f32");
    ctx.bind<test>("test")
        .constructor()
        .prop("x", &test::x)//, &test::get_x, &test::set_x)
    .finalize();

    be.commit_bindings();

    // printf("------------IR code-------------\n");


    script_module* mod = ctx.add_code(
        "test",
        "i32 a = 1;\n"
        "a += 5;\n"
        "print(1.2345f.toFixed(4));\n"
        "class obj {\n"
            "constructor(f32 e) {\n"
                "this.x = e;\n"
            "}\n"
            "void print() { print_f32(0, this.x); }\n"
            "f32 x;\n"
        "};\n"
    );

    if (!mod) {
        print_log(&ctx);
        return -1;
    }

    script_object obj(&ctx, mod->types()->get("obj"), 3.14f);
    property_accessor<f32> v = obj.prop<f32>("x");
    v = 1.23f;
    v = obj.prop<f32>("x");
    obj.call<void>("print", nullptr);

    script_object obj1(&ctx, ctx.global()->types()->get<test>());
    f32& x = obj1.prop<f32>("x");
    x = 1.234f;


    printf("------------VM code-------------\n");
    //print_code((vm_backend*)ctx.generator());

    printf("-------------result-------------\n");
    // ((vm_backend*)ctx.generator())->log_instructions(true);
    mod->init();

    return 0;
}