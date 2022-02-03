#include <gjs/gjs.h>
#include <gjs/gjs.hpp>

#include <stdio.h>

using namespace gjs;

void print_help() {
    printf("Usage:\n");
    printf("\tgjs [options] <file> [file arguments].\n");
    printf("Options:\n");
    printf("\t -b -backend [vm | native]\tSpecifies which backend to use for code execution\n");
    printf("\t -log-asm\t\t\tLogs the assembly code that scripts get compiled to\n");
    printf("\t -log-ir\t\t\tLogs the intermediate code that scripts get compiled to\n");
    printf("\t -log-vme\t\t\tLogs the VM instructions during execution\n");
    printf("\t -m -mem [bytes]\t\tSpecifies the amount of memory that scripts have access to\n\t\t\t\t\t\t(Size must be >= 1024 B and <= 128 MB, currently not respected)\n");
    printf("\t -s -stack [bytes]\t\tSpecifies the size of the stack used by scripts\n\t\t\t\t\t\t(Size must be >= 1024 B and <= 128 MB, VM only)\n");
    printf("\t -no-run\t\t\tOnly compiles the specified script without executing it\n\t\t\t\t\t\t(Still executes global code from imported modules)\n");
    printf("Note:\n");
    printf("\tNo arguments specified before <file> will be provided to the scripts through the global process variable\n");
}

int print (const script_string& s) {
    return printf("%s\n", s.c_str());
}

void bind_ctx(script_context* ctx) {
    //ctx->bind(print, "print");
}

vm_allocator* g_allocator = nullptr;
bool log_ir = false;
bool log_asm = false;
bool dont_run = false;

bool do_print_help(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-h" || args[i] == "--h" || args[i] == "-help" || args[i] == "--help") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

u32 get_stack_size(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-s" || args[i] == "--s" || args[i] == "-stack" || args[i] == "--stack") {
            if (i + 1 >= args.size()) {
                throw std::exception("The stack size parameter was specified without providing an argument. Usage '--s <size in bytes>'. Size must be >= 1024 B and <= 128 MB"); 
            }
            u32 sz = atoi(args[i + 1].c_str());
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

u32 get_mem_size(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-m" || args[i] == "--m" || args[i] == "-mem" || args[i] == "--mem") {
            if (i + 1 >= args.size()) {
                throw std::exception("The memory size parameter was specified without providing an argument. Usage '--m <size in bytes>'. Size must be >= 1024 B and <= 128 MB"); 
            }
            u32 sz = atoi(args[i + 1].c_str());
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

bool do_log_ir(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-log-ir" || args[i] == "--log-ir") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_log_asm(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-log-asm" || args[i] == "--log-asm") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_log_vme(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-log-vme" || args[i] == "--log-vme") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool do_not_run(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-no-run" || args[i] == "--no-run") {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}


backend* create_backend(std::vector<std::string>& args) {
    for (u32 i = 0;i < args.size();i++) {
        if (args[i][0] != '-') break;
        if (args[i] == "-b" || args[i] == "--b" || args[i] == "-backend" || args[i] == "--backend") {
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
                x86_backend* be = new x86_backend();
                if (do_log_vme(args)) throw std::exception("--log-vme cannot be used with the native backend");
                // if (do_log_native_ir(args)) be->
                return be;
            } else {
                throw std::exception(std::string("The backend parameter was given an invalid argument '" + args[i + 1] + "'. Usage '--b [vm | native]").c_str());
            }
        }
    }

    return new x86_backend();
}

script_context* create_ctx(std::vector<std::string>& args) {
    gjs::backend* backend = create_backend(args);
    log_ir = do_log_ir(args);
    log_asm = do_log_asm(args);
    dont_run = do_not_run(args);

    if (do_print_help(args)) {
        print_help();
        return nullptr;
    }

    if (args.size() == 0) {
        printf("No script specified. Usage: gjs [options] <file> [file arguments]. Use -h for more info.\n");
        if (backend) delete backend;
        if (g_allocator) delete g_allocator;
        return nullptr;
    }

    if (args[0][0] == '-') {
        printf("Unrecognized argument: '%s'\n", args[0].c_str());
        return nullptr;
    }

    // all args used to configure the context must be consumed
    // before this point or they will be passed to the script
    // itself
    std::vector<const char*> argp;
    for (u32 i = 1;i < args.size();i++) argp.push_back(args[i].c_str());

    script_context* ctx = new script_context(argp.size(), argp.data(), backend);
    if (log_ir) ctx->compiler()->add_ir_step(debug_ir_step);

    return ctx;
}

script_context* parse_args(std::vector<std::string>& args) {
    script_context* ctx = create_ctx(args);

    return ctx;
}

int main(i32 argc, const char** argp) {
    try {
        std::vector<std::string> args;
        for (i32 i = 1;i < argc;i++) {
            args.push_back(argp[i]);
        }

        script_context* ctx = parse_args(args);
        if (!ctx) return -1;

        ctx->io()->set_cwd_from_args(argc, argp);

        bind_ctx(ctx);
        ctx->generator()->commit_bindings();

        script_module* mod = ctx->resolve(args[0]);
        if (!mod) {
            print_log(ctx);
            return -1;
        }

        if (log_asm && g_allocator) print_code((vm_backend*)ctx->generator());

        if (!dont_run) mod->init();

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