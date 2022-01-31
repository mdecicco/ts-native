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
script_function* balls = nullptr;
void brk(callback<bool(*)(i32, i32)> cb) {
    if (cb(4, 4)) {
        printf("double fuck you.\n");
    }
    if (balls) call(balls, nullptr, cb);
}

bool t (i32 a, i32 b) { return a == b; }

int main(int arg_count, const char** args) {
    srand(time(nullptr));

    //basic_malloc_allocator alloc;
    //vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    x86_backend be;
    script_context ctx(&be);
    bind(&ctx, brk, "fuck");
    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);
    // ctx.compiler()->add_ir_step(optimize::ir_phase_1, false);
    // ctx.compiler()->add_ir_step(optimize::dead_code, false);
    ctx.compiler()->add_ir_step(debug_ir_step, false);

    brk(t);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        // print_log(&ctx);
        return -1;
    }
    // print_code(&be);
    // be.log_instructions(true);
    // be.log_lines(true);

    mod->init();
    balls = mod->function("test");

    u64 t = UINT64_MAX - 1;
    u64 v = call(mod->function("main"), nullptr, UINT64_MAX - 2);
    bool a = t == v;
    bool grr = call(mod->function("test"), nullptr, [](i32 a, i32 b) {
        return a == b;
    });

    shutdown_ui();

    return 0;
}
