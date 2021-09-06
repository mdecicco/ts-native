#include <gjs/gjs.h>
#include <stdio.h>

using namespace gjs;

int main(int arg_count, const char** args) {
    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);
    // ctx.compiler()->add_ir_step(debug_ir_step, false);
    // ctx.compiler()->add_ir_step(debug_ir_step, true);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    print_code(&be);
    // be.log_instructions(true);

    mod->init();

    mod->function("test")->call(nullptr);
    return 0;
}