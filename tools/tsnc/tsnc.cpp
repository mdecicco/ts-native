#include <tsn/tsn.h>
#include <tsn/common/Context.h>
#include <tsn/io/Workspace.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Output.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/vm/VMBackend.h>
#include <tsn/vm/Instruction.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>
#include <utils/json.hpp>


#include <filesystem>
#include "tojson.h"

using namespace tsn;
using namespace compiler;
using namespace utils;
using namespace nlohmann;

#define EXIT_EARLY                       1
#define COMPILATION_SUCCESS              0
#define COMPILATION_ERROR               -1
#define UNKNOWN_ERROR                   -2
#define FAILED_TO_OPEN_FILE             -3
#define FILE_EMPTY                      -4
#define FAILED_TO_READ_FILE             -5
#define FAILED_TO_ALLOCATE_INPUT_BUFFER -6
#define ARGUMENT_ERROR                  -7
#define CONFIG_PARSE_ERROR              -8
#define CONFIG_VALUE_ERROR              -9

enum output_mode {
    om_all,
    om_ast,
    om_funcs,
    om_types,
    om_logs,
    om_code,
    om_backend,
    om_exec
};

enum backend_type {
    bt_none,
    bt_vm
};

struct tsnc_config {
    const char* script_path;
    const char* config_path;
    output_mode mode;
    backend_type backend;
    bool pretty_print;
    bool debugLogging;
    bool disableOptimizations;
};

void print_help();
const char* getStringArg(i32 argc, const char** argv, const char* code);
bool getBoolArg(i32 argc, const char** argv, const char* code);
i32 parse_args(i32 argc, const char** argv, tsnc_config* conf, Config* ctxConf);
char* loadText(const char* filename, bool required, const tsnc_config& conf);
i32 processConfig(const json& configIn, Config& configOut, const tsnc_config& tsncConf);
i32 handleResult(Context* ctx, Module* mod, const tsnc_config& conf);
void initError(const char* err, const tsnc_config& conf);

i32 main (i32 argc, const char** argv) {
    if (argc > 0) {
        std::string progPath = argv[0];
        size_t idx = progPath.find_last_of('/');
        if (idx == std::string::npos) idx = progPath.find_last_of('\\');
        std::string cwd = progPath.substr(0, idx);
        std::filesystem::current_path(cwd);
    }

    tsnc_config conf;
    conf.script_path = "main";
    conf.config_path = "./tsnc.json";
    conf.mode = om_all;
    conf.pretty_print = !getBoolArg(argc, argv, "m");
    conf.backend = bt_none;
    i32 status = 0;
    Config contextCfg;

    try {
        char* configInput = loadText(conf.config_path, false, conf);
        if (configInput) {
            json config = json::parse(configInput);
            delete [] configInput;

            status = processConfig(config, contextCfg, conf);
            if (status != 0) return status;

            conf.debugLogging = contextCfg.debugLogging;
            conf.disableOptimizations = contextCfg.disableOptimizations;

            status = parse_args(argc, argv, &conf, &contextCfg);
            if (status != 0) return status;
        }
    } catch (i32 error) {
        return error;
    } catch (std::exception exc) {
        initError((std::string("Encountered exception while parsing config: ") + exc.what()).c_str(), conf);
        return CONFIG_PARSE_ERROR;
    }
    
    contextCfg.debugLogging = conf.debugLogging;
    contextCfg.disableOptimizations = conf.disableOptimizations;
    contextCfg.disableExecution = true;

    utils::Mem::Create();
    tsn::ffi::ExecutionContext::Init();

    {
        Context ctx = Context(1, &contextCfg);
        
        backend::IBackend* be = nullptr;
        switch (conf.backend) {
            case bt_none: break;
            case bt_vm: be = new vm::Backend(&ctx, 16384);
        }

        ctx.init(be);

        Module* mod = ctx.getModule(conf.script_path);
        if (be) mod->init();

        status = handleResult(&ctx, mod, conf);
        
        ctx.shutdown();
        if (be) delete be;
    }

    tsn::ffi::ExecutionContext::Shutdown();
    utils::Mem::Destroy();
    return status;
}

void print_help() {
    printf("TSN Compiler version 0.0.2\n");
    printf("Info:\n");
    printf("    This tool is used to compile and/or validate TSN scripts. It will attempt to\n");
    printf("    compile the provided script, and subsequently output metadata about the script\n");
    printf("    in the JSON format. This metadata includes the abstract syntax tree, symbols,\n");
    printf("    log messages, data types, and IR code.\n");
    printf("Usage:\n");
    printf("    -s <module path>    Specifies entrypoint module to compile. Defaults to 'main'.\n");
    printf("    -c <config file>    Specifies compiler configuration file, must be JSON format. Defaults to './tsnc.json'.\n");
    printf("    -m                  Outputs minified json instead of pretty-printed json.\n");
    printf("    -d                  Enables debug log messages (overrides configuration file)\n");
    printf("    -u                  Disables optimization (overrides configuration file)\n");
    printf("    -b <backend>        Specifies the backend to use for running compiled code.\n");
    printf("                            none     Does not run code (default)\n");
    printf("                            vm       Uses a register-based VM to run code\n");
    printf("    -o <mode>           Specifies the output mode. See list below for available modes.\n");
    printf("                            all      Outputs all available compilation output (default)\n");
    printf("                            ast      Outputs only the abstract syntax tree.\n");
    printf("                            funcs    Outputs only list of functions.\n");
    printf("                            types    Outputs only list of types.\n");
    printf("                            code     Outputs only the IR code.\n");
    printf("                            logs     Outputs only the logs.\n");
    printf("                            backend  Outputs only backend data.\n");
    printf("                            exec     Outputs only what the script itself outputs.\n");
    printf("    -h                  Prints this information.\n");
}

const char* getStringArg(i32 argc, const char** argv, const char* code) {
    for (i32 i = 0;i < argc;i++) {
        if (strlen(argv[i]) <= 1 || argv[i][0] != '-') continue;
        if (strcmp(argv[i] + 1, code) == 0) {
            if (i + 1 >= argc) {
                printf("Invalid arguments.\n\n");
                print_help();
                throw ARGUMENT_ERROR;
            }

            if (strlen(argv[i + 1]) == 0 || argv[i + 1][0] == '-') {
                printf("Invalid arguments.\n\n");
                print_help();
                throw ARGUMENT_ERROR;
            }

            return argv[i + 1];
        }
    }

    return nullptr;
}

bool getBoolArg(i32 argc, const char** argv, const char* code) {
    for (i32 i = 0;i < argc;i++) {
        if (strlen(argv[i]) <= 1 || argv[i][0] != '-') continue;
        if (strcmp(argv[i] + 1, code) == 0) return true;
    }

    return false;
}

i32 parse_args(i32 argc, const char** argv, tsnc_config* conf, Config* ctxConf) {
    try {
        const char* tmp = nullptr;

        tmp = getStringArg(argc, argv, "s");
        if (tmp) conf->script_path = tmp;

        tmp = getStringArg(argc, argv, "c");
        if (tmp) conf->config_path = tmp;

        tmp = getStringArg(argc, argv, "b");
        if (tmp) {
            if (strcmp(tmp, "none") == 0) conf->backend = bt_none;
            else if (strcmp(tmp, "vm") == 0) conf->backend = bt_vm;
            else {
                printf("Invalid backend option '%s'.\n", tmp);
                print_help();
                return ARGUMENT_ERROR;
            }
        }

        tmp = getStringArg(argc, argv, "o");
        if (tmp) {
            if (strcmp(tmp, "all") == 0) conf->mode = om_all;
            else if (strcmp(tmp, "ast") == 0) conf->mode = om_ast;
            else if (strcmp(tmp, "funcs") == 0) conf->mode = om_funcs;
            else if (strcmp(tmp, "types") == 0) conf->mode = om_types;
            else if (strcmp(tmp, "code") == 0) conf->mode = om_code;
            else if (strcmp(tmp, "backend") == 0) conf->mode = om_backend;
            else if (strcmp(tmp, "logs") == 0) conf->mode = om_logs;
            else if (strcmp(tmp, "exec") == 0) conf->mode = om_exec;
            else {
                printf("Invalid output option '%s'.\n", tmp);
                print_help();
                return ARGUMENT_ERROR;
            }

            if (conf->mode == om_backend && conf->backend == bt_none) {
                printf("Cannot output backend data when no backend is selected. Please use -b option to specify backend.\n");
                print_help();
                return ARGUMENT_ERROR;
            }

            if (conf->mode == om_exec && conf->backend == bt_none) {
                printf("Cannot output execution output when no backend is selected. Please use -b option to specify backend.\n");
                print_help();
                return ARGUMENT_ERROR;
            }
        }

        if (getBoolArg(argc, argv, "m")) conf->pretty_print = false;
        if (getBoolArg(argc, argv, "d")) conf->debugLogging = true;
        if (getBoolArg(argc, argv, "u")) conf->disableOptimizations = true;

        if (getBoolArg(argc, argv, "h")) {
            print_help();
            return EXIT_EARLY;
        }
    } catch(i32 code) {
        return code;
    }

    return 0;
}

char* loadText(const char* filename, bool required, const tsnc_config& conf) {
    FILE* fp = nullptr;
    #ifdef _MSC_VER
    fopen_s(&fp, filename, "r");
    #else
    fp = fopen(filename, "r");
    #endif

    if (!fp) {
        if (!required) return nullptr;

        initError(utils::String::Format("Unable to open file '%s'", filename).c_str(), conf);
        throw FAILED_TO_OPEN_FILE;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz == 0) {
        initError(utils::String::Format("File '%s' is empty", filename).c_str(), conf);
        fclose(fp);
        throw FILE_EMPTY;
    }

    char* code = new char[sz + 1];
    if (!code) {
        initError(utils::String::Format("Failed to allocate input buffer for '%s'", filename).c_str(), conf);
        fclose(fp);
        throw FAILED_TO_ALLOCATE_INPUT_BUFFER;
    }

    size_t readSz = fread(code, 1, sz, fp);
    if (readSz != sz && !feof(fp)) {
        delete [] code;
        initError(utils::String::Format("Failed to read '%s'", filename).c_str(), conf);
        fclose(fp);
        throw FAILED_TO_READ_FILE;
    }
    code[readSz] = 0;
    
    fclose(fp);

    return code;
}

i32 processConfig(const json& configIn, Config& configOut, const tsnc_config& tsncConf) {
    if (configIn.contains("directories")) {
        json dirs = configIn["directories"];
        if (!dirs.empty()) {
            if (dirs.contains("root")) {
                json wsroot = dirs["root"];
                if (!wsroot.empty()) {
                    if (!wsroot.is_string()) {
                        initError("config.directories.root is not a string. It should be the path to the root of the script workspace", tsncConf);
                        return CONFIG_VALUE_ERROR;
                    }
                    configOut.workspaceRoot = wsroot;
                }
            }

            if (dirs.contains("support")) {
                json supportDir = dirs["support"];
                if (!supportDir.empty()) {
                    if (!supportDir.is_string()) {
                        initError("config.directories.support is not a string. It should be the path to the folder where the compiler will store persistence data", tsncConf);
                        return CONFIG_VALUE_ERROR;
                    }
                    configOut.supportDir = supportDir;
                }
            }
        }
    }

    if (configIn.contains("enableDebugLogs")) {
        json debug = configIn["enableDebugLogs"];
        if (!debug.empty()) {
            if (!debug.is_boolean()) {
                initError("config.enableDebugLogs is not a boolean.", tsncConf);
                return CONFIG_VALUE_ERROR;
            }

            configOut.debugLogging = debug;
        }
    }

    if (configIn.contains("disableOptimizations")) {
        json disableOpt = configIn["disableOptimizations"];
        if (!disableOpt.empty()) {
            if (!disableOpt.is_boolean()) {
                initError("config.disableOptimizations is not a boolean.", tsncConf);
                return CONFIG_VALUE_ERROR;
            }

            configOut.disableOptimizations = disableOpt;
        }
    }

    return 0;
}

void initError(const char* err, const tsnc_config& conf) {
    json log;
    log["code"] = json(nullptr);
    log["type"] = "error";
    log["range"] = json(nullptr);
    log["line_txt"] = json(nullptr);
    log["line_idx"] = json(nullptr);
    log["message"] = err;
    log["ast"] = json(nullptr);

    json j;
    j["logs"] = json::array({ log });
    j["symbols"] = json::array();
    j["types"] = json::array();
    j["functions"] = json::array();
    j["globals"] = json::array();
    j["ast"] = json(nullptr);

    if (conf.pretty_print) printf(j.dump(4).c_str());
    else printf(j.dump().c_str());
}

i32 handleResult(Context* ctx, Module* mod, const tsnc_config& conf) {
    bool hadErrors = ctx->getPipeline()->getLogger()->hasErrors();

    if (mod && !hadErrors && conf.mode == om_exec) {
        // The module already ran and output anything it was going to output
        return COMPILATION_SUCCESS;
    }

    static const char* access[] = { "public", "private", "trusted" };
    const auto& logs = ctx->getPipeline()->getLogger()->getMessages();
    auto c = ctx->getPipeline()->getCompiler();

    json out;
    switch (conf.mode) {
        case om_all: {
            out["logs"] = json::array();
            out["symbols"] = json::array();
            out["types"] = json::array();
            out["functions"] = json::array();
            out["globals"] = json::array();
            out["backend"] = json(nullptr);
            out["ast"] = json(nullptr);
            break;
        }
        case om_ast: {
            out["logs"] = json::array();
            out["ast"] = json(nullptr);
            break;
        }
        case om_funcs: {
            out["logs"] = json::array();
            out["functions"] = json::array();
            break;
        }
        case om_types: {
            out["logs"] = json::array();
            out["types"] = json::array();
            break;
        }
        case om_code: {
            out["logs"] = json::array();
            out["code"] = json::array();
            break;
        }
        case om_logs: {
            out["logs"] = json::array();
            break;
        }
        case om_backend: {
            out["logs"] = json::array();
            out["backend"] = json(nullptr);
            break;
        }
    }

    for (u32 i = 0;i < logs.size();i++) {
        out["logs"].push_back(toJson(logs[i]));
    }

    if (c && c->getOutput()) {
        if (out.contains("symbols")) {
            const auto& syms = c->getOutput()->getSymbolLifetimeData();
            for (u32 i = 0;i < syms.size();i++) {
                if (syms[i].sym->getFlags().is_type && syms[i].sym->getType()->getInfo().is_host) continue;
                if (syms[i].sym->getFlags().is_type && syms[i].sym->getType()->getInfo().is_function) continue;
                if (syms[i].sym->getFlags().is_function && syms[i].sym->isImm()) continue;
                if (syms[i].name.firstIndexOf('@') != -1) continue;
                out["symbols"].push_back(toJson(syms[i]));
            }
        }

        if (out.contains("types")) {
            const auto& types = c->getOutput()->getTypes();
            for (u32 i = 0;i < types.size();i++) {
                out["types"].push_back(toJson(types[i], false));
            }
        }

        if (out.contains("functions")) {
            const auto& funcs = c->getOutput()->getFuncs();
            for (u32 i = 0;i < funcs.size();i++) {
                if (!funcs[i] || !funcs[i]->getOutput()) continue;
                out["functions"].push_back(toJson(funcs[i]->getOutput(), false, funcs[i]));
            }
        }

        if (out.contains("globals")) {
            const auto& globals = c->getOutput()->getModule()->getData();
            for (u32 i = 0;i < globals.size();i++) {
                out["globals"].push_back(toJson(i, globals[i]));
            }
        }

        if (out.contains("code")) {
            json &o = out["code"];
            auto& output = ctx->getPipeline()->getCompilerOutput()->getCode();
            auto& funcs = c->getOutput()->getFuncs();
            for (u32 i = 0;i < output.size();i++) {
                if (!output[i] || !output[i]->owner) continue;

                ffi::Function* fn = output[i]->owner;
                FunctionDef* fd = funcs.find([fn](FunctionDef* fd) {
                    return fd->getOutput() == fn;
                });

                json func = toJson(fn, true, fd);
                json& oc = func["code"] = json::array();

                const auto& code = output[i]->code;
                u32 digitCount = 0;
                u32 x = code.size();
                while (x) {
                    x /= 10;
                    digitCount++;
                }

                char lnFmt[16] = { 0 };
                snprintf(lnFmt, 16, "[%%-%dd] %%s", digitCount);

                for (u32 c = 0;c < code.size();c++) {
                    utils::String s = utils::String::Format(lnFmt, c, code[c].toString(ctx).c_str());
                    oc.push_back(s.c_str());
                }

                o.push_back(func);
            }
        }
    
        if (out.contains("backend")) {
            json backend;
            switch (conf.backend) {
                case bt_none: {
                    backend["type"] = "none";
                    break;
                }
                case bt_vm: {
                    vm::Backend* be = (vm::Backend*)ctx->getBackend();

                    const auto& funcs = be->allFunctions();
                    const auto& instrs = be->getCode();
                    json out_funcs = json::array();

                    for (u32 f = 0;f < funcs.size();f++) {
                        const auto* range = be->getFunctionData(funcs[f]);
                        if (range) {
                            FunctionDef* fd = nullptr;
                            const auto& funcDefs = ctx->getPipeline()->getCompiler()->getOutput()->getFuncs();
                            for (u32 of = 0;of < funcDefs.size();of++) {
                                if (funcDefs[of]->getOutput() == funcs[f]) {
                                    fd = funcDefs[of];
                                    break;
                                }
                            }

                            json code = json::array();
                            for (u32 i = range->begin;i < range->end;i++) {
                                FunctionDef* fn = c->getOutput()->getFuncs().find([i](FunctionDef* f){
                                    return reinterpret_cast<u64>(f->getOutput()->getAddress()) == u64(i);
                                });
                                
                                code.push_back(utils::String::Format("[0x%03x] %s", i, instrs[i].toString(ctx).c_str()).c_str());
                            }
                            
                            json func = toJson(funcs[f], true, fd);
                            func["code"] = code;
                            out_funcs.push_back(func);
                        }
                    }


                    backend["type"] = "vm";
                    backend["functions"] = out_funcs;
                    break;
                }
            }

            out["backend"] = backend;
        }
    }

    if (ctx->getPipeline()->getAST() && out.contains("ast")) {
        out["ast"] = toJson(ctx->getPipeline()->getAST());
    }

    if (conf.pretty_print) printf(out.dump(4).c_str());
    else printf(out.dump().c_str());

    if (hadErrors) return COMPILATION_ERROR;
    return COMPILATION_SUCCESS;
}