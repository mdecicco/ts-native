#include <gjs/gjs.h>
#include <stdio.h>
#include <gjs/backends/b_win_x86_64.h>

using namespace gjs;

int main(int arg_count, const char** args) {
    win64_backend be;
    script_context ctx(&be);

    be.commit_bindings();
    be.log_ir(true);
    ctx.compiler()->add_ir_step(debug_ir_step);
    ctx.io()->set_cwd_from_args(arg_count, args);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }

    mod->init();
    struct f0 {
        i32 a, b, c;
    };
    struct f1 {
        f0 a, b, c;
    };

    f1* y = (f1*)mod->local_ptr("y");

    /*
    script_function* f = mod->function<i32>("main");
    if (f) {
        i32 b = 0;
        ctx.call(f, &b);
        printf("Script finished with return value %d\n", b);
    }
    else printf("Script has no 'i32 main()' function\n");
    */

    return 0;
}