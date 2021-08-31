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

i32 testCb (callback<i32(*)(i32, i32)> cb) {
    i32 x = cb(1, 2);
    return x;
}

int main(int arg_count, const char** args) {
    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    ctx.bind(testCb, "testCb");

    be.commit_bindings();
    //be.log_ir(true);
    ctx.io()->set_cwd_from_args(arg_count, args);
    //ctx.compiler()->add_ir_step(remove_unused_regs_pass);
    ctx.compiler()->add_ir_step(debug_ir_step);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    print_code(&be);
    //be.log_instructions(true);

    mod->init();

    return 0;
}