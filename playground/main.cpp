#include <gjs/gjs.h>
#include <stdio.h>
#include <gjs/backends/b_win_x86_64.h>

using namespace gjs;

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

int main(int arg_count, const char** args) {
    win64_backend be;
    script_context ctx(&be);

    be.commit_bindings();
    be.log_ir(true);
    ctx.io()->set_cwd_from_args(arg_count, args);
    ctx.compiler()->add_ir_step(remove_unused_regs_pass);
    ctx.compiler()->add_ir_step(debug_ir_step);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }

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

    return 0;
}