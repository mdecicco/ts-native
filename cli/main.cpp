#include <gjs/gjs.h>
#include <stdio.h>
#include <gjs/backends/b_win_x86_64.h>
#include <gjs/builtin/script_math.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <gjs/builtin/script_vec4.h>

using namespace gjs;
using namespace gjs::math;

int print (const script_string& s) {
    return printf("%s\n", s.c_str());
}

void bind_ctx(script_context* ctx) {
    //ctx->bind(print, "print");
}

void remove_unused_regs_pass(script_context* ctx, compilation_output& in) {
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

vm_allocator* g_allocator = nullptr;

u64 get_stack_size(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-s") {
            if (i + 1 >= args.size()) {
                throw std::exception("The stack size parameter was specified without providing an argument. Usage '--s <size in bytes>'. Size must be >= 1024 B and <= 128 MB"); 
            }
            int sz = atoi(args[i + 1].c_str());
            if (sz < 1024 || sz > 128 * 1024 * 1024) {
                throw std::exception(std::string("The stack size parameter was given an invalid argument '" + args[i + 1] + "'. Usage '--s <size in bytes>'. Size must be >= 1024 B and <= 128 MB").c_str());
            }
            args.erase(args.begin() + i + 1);
            args.erase(args.begin() + i);

            return sz;
        }
    }
    return 8 * 1024 * 1024;
}

u64 get_mem_size(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-s") {
            if (i + 1 >= args.size()) {
                throw std::exception("The memory size parameter was specified without providing an argument. Usage '--m <size in bytes>'. Size must be >= 1024 B and <= 128 MB"); 
            }
            int sz = atoi(args[i + 1].c_str());
            if (sz < 1024 || sz > 128 * 1024 * 1024) {
                throw std::exception(std::string("The memory size parameter was given an invalid argument '" + args[i + 1] + "'. Usage '--m <size in bytes>'. Size must be >= 1024 B and <= 128 MB").c_str());
            }
            args.erase(args.begin() + i + 1);
            args.erase(args.begin() + i);

            return sz;
        }
    }
    return 8 * 1024 * 1024;
}

bool do_log_native_ir(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-log-native-ir") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_log_ir(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-log-ir") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_log_vmi(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-log-vmi") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_log_vme(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-log-vm-exec") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

backend* create_backend(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i] == "-b") {
            if (i + 1 >= args.size()) {
                throw std::exception("The backend parameter was specified without providing an argument. Usage '--b [vm | native]'"); 
            }
            std::transform(args[i + 1].begin(), args[i + 1].end(), args[i + 1].begin(), ::tolower);
            if (args[i + 1] == "vm") {
                args.erase(args.begin() + i + 1);
                args.erase(args.begin() + i);
                g_allocator = new basic_malloc_allocator();
                vm_backend* be = new vm_backend(g_allocator, get_stack_size(args), get_mem_size(args));
                if (do_log_vme(args)) be->log_instructions(true);
                return be;
            } else if (args[i + 1] == "native") {
                args.erase(args.begin() + i + 1);
                args.erase(args.begin() + i);
                win64_backend* be = new win64_backend();
                if (do_log_vme(args)) throw std::exception("--log-vm-exec cannot be used with the native backend");
                if (do_log_native_ir(args)) be->log_ir(true);
                return be;
            } else {
                throw std::exception(std::string("The backend parameter was given an invalid argument '" + args[i + 1] + "'. Usage '--b [vm | native]").c_str());
            }
        }
    }

    return new win64_backend();
}

script_context* create_ctx(std::vector<std::string>& args) {
    script_context* ctx = new script_context(create_backend(args));
    //ctx->compiler()->add_ir_step(remove_unused_regs_pass);
    if (do_log_ir(args)) {
        ctx->compiler()->add_ir_step(debug_ir_step);
    }

    return ctx;
}

script_context* parse_args(std::vector<std::string>& args) {
    script_context* ctx = create_ctx(args);

    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] == '-') {
            throw std::exception(("Unrecognized argument: '" + args[i] + "'").c_str());
        }
    }

    return ctx;
}

void test(void (*cb)(i32, f32)) {
    cb(1, 1.0f);
}

int main(i32 argc, const char** argp) {
    try {
        std::vector<std::string> args;
        for (u32 i = 1;i < argc;i++) {
            args.push_back(argp[i][0] == '-' ? argp[i] + 1 : argp[i]);
        }

        bool log_vmi = do_log_vmi(args);
        script_context* ctx = parse_args(args);
        ctx->io()->set_cwd_from_args(argc, argp);

        script_type* tp = ctx->types()->get<void(*)(int, int)>();
        script_function* func = ctx->bind(test, "test");

        if (log_vmi && !g_allocator) {
            throw std::exception("--log-vmi cannot be used with the native backend");
        }

        bind_ctx(ctx);
        ctx->generator()->commit_bindings();

        if (args.size() == 0) {
            printf("No script specified. Usage gjs [options] file [file arguments]. Use gjs --help for more information.\n");
            backend* be = ctx->generator();
            delete ctx;
            delete be;
            if (g_allocator) delete g_allocator;
            return -1;
        }

        script_module* mod = ctx->resolve(args[0]);
        if (!mod) {
            print_log(ctx);
            return -1;
        }

        if (log_vmi) print_code((vm_backend*)ctx->generator());

        mod->init();

        backend* be = ctx->generator();
        delete ctx;
        delete be;
        if (g_allocator) delete g_allocator;
    } catch (const gjs::error::runtime_exception& e) {
        printf("Runtime exception: [%d] %s\n", e.code, e.message.c_str());
        if (e.src.module.length() > 0) printf("In module: %s\n", e.src.module.c_str());
        if (e.src.line_text.length() > 0) {
            printf("At line:\n%d:%d: %s\n", e.src.line, e.src.col, e.src.line_text.c_str());
        }
        return -1;
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return -1;
    }

    return 0;
}