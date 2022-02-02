#include <gjs/gjs.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/gjs.hpp>

#include <stdio.h>
#include <time.h>
#include <chrono>
#include <thread>

#include "ui.h"

using namespace gjs;

int main(int arg_count, const char** args) {
    srand(time(nullptr));

    // basic_malloc_allocator alloc;
    // vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    x86_backend be;
    script_context ctx(&be);

    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);
    ctx.compiler()->add_ir_step(optimize::ir_phase_1, false);
    ctx.compiler()->add_ir_step(optimize::dead_code, false);
    // ctx.compiler()->add_ir_step(debug_ir_step, false);


    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    // print_code(&be);
    // be.log_instructions(true);
    // be.log_lines(true);

    mod->init();

    call(mod->function("main"));

    shutdown_ui();

    return 0;
}
