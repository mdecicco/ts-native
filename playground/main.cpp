#include <gjs.h>
#include <stdio.h>

using namespace gjs;

void print_f32(u16 i, f32 f) {
    printf("%d: %f\n", i, f);
}

// https://www.notion.so/3ccc9d8dba114bf8acfbbe238cb729e3?v=1a0dff3b20ba4f678bc7f5866665c4df
int main(int arg_count, const char** args) {
    std::string dir = "";
    auto dp = split(args[0], "/\\");
    for (u16 i = 0;i < dp.size() - 1;i++) dir += dp[i] + "/";

    script_context ctx;
    ctx.io()->set_cwd(dir);

    // ctx.compiler()->add_ir_step(debug_ir_step);

    ctx.bind(print_f32, "print_f32");

    // printf("------------IR code-------------\n");


    script_module* mod = ctx.add_code(
        "test",
        "print(1.2345f.toFixed(4));\n"
    );

    if (!mod) {
        print_log(&ctx);
        return -1;
    }


    printf("------------VM code-------------\n");
    print_code((vm_backend*)ctx.generator());

    printf("-------------result-------------\n");
    // ((vm_backend*)ctx.generator())->log_instructions(true);
    mod->init();

    return 0;
}