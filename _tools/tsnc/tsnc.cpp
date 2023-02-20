#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/common/Module.h>
#include <tsn/io/Workspace.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Output.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/pipeline/Pipeline.h>

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
    om_code
};

struct tsnc_config {
    const char* script_path;
    const char* config_path;
    output_mode mode;
    bool pretty_print;
    bool trusted;
    bool debugLogging;
    bool disableOptimizations;
};

void print_help();
const char* getStringArg(i32 argc, const char** argv, const char* code);
bool getBoolArg(i32 argc, const char** argv, const char* code);
i32 parse_args(i32 argc, const char** argv, tsnc_config* conf, Config* ctxConf);
char* loadText(const char* filename, bool required, const tsnc_config& conf);
i32 processConfig(const json& configIn, Config& configOut, const tsnc_config& tsncConf);
i32 handleResult(Context* ctx, const script_metadata& meta, const tsnc_config& conf);
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
    Config contextCfg;
    i32 status = parse_args(argc, argv, &conf, &contextCfg);
    if (status != 0) {
        return status;
    }

    try {
        char* configInput = loadText(conf.config_path, false, conf);
        if (configInput) {
            json config = json::parse(configInput);
            delete [] configInput;

            status = processConfig(config, contextCfg, conf);
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
    
    script_metadata meta;
    try {
        meta.path = std::filesystem::relative(conf.script_path, contextCfg.workspaceRoot.c_str()).string();
        enforceDirSeparator(meta.path);
        meta.size = (size_t)std::filesystem::file_size(conf.script_path);
        meta.is_trusted = conf.trusted;
        meta.is_external = true;
        meta.cached_on = 0;
        meta.modified_on = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::filesystem::last_write_time(conf.script_path).time_since_epoch()
        ).count();
    } catch (std::exception exc) {
        initError((std::string("Encountered exception while getting script metadata: ") + exc.what()).c_str(), conf);
        return UNKNOWN_ERROR;
    }

    if (meta.size == 0) {
        initError("The specified script is empty", conf);
        return FILE_EMPTY;
    }

    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    {
        Context ctx = Context(1, &contextCfg);
        ctx.getPipeline()->buildFromSource(&meta);
        status = handleResult(&ctx, meta, conf);
    }

    utils::Mem::Destroy();
    utils::String::Allocator::Destroy();

    return status;
}

void print_help() {
    printf("TSN Compiler version 0.0.1\n");
    printf("Info:\n");
    printf("    This tool is used to compile and/or validate TSN scripts. It will attempt to\n");
    printf("    compile the provided script, and subsequently output metadata about the script\n");
    printf("    in the JSON format. This metadata includes the abstract syntax tree, symbols,\n");
    printf("    log messages, data types, and IR code.\n");
    printf("Usage:\n");
    printf("    -s <script file>    Specifies entrypoint script to compile. Defaults to './main.tsn'.\n");
    printf("    -c <config file>    Specifies compiler configuration file, must be JSON format. Defaults to './tsnc.json'.\n");
    printf("    -m                  Outputs minified json instead of pretty-printed json.\n");
    printf("    -t                  Compiles the specified script in trusted mode\n");
    printf("    -d                  Enables debug log messages (overrides configuration file)\n");
    printf("    -u                  Disables optimization (overrides configuration file)\n");
    printf("    -o <mode>           Specifies the output mode. See list below for available modes.\n");
    printf("                            all      Outputs all available compilation output (default)\n");
    printf("                            ast      Outputs only the abstract syntax tree.\n");
    printf("                            funcs    Outputs only list of functions.\n");
    printf("                            types    Outputs only list of types.\n");
    printf("                            code     Outputs only the IR code.\n");
    printf("                            logs     Outputs only the logs.\n");
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
    conf->script_path = "./main.tsn";
    conf->config_path = "./tsnc.json";
    conf->mode = om_all;
    conf->pretty_print = true;
    conf->trusted = false;
    conf->debugLogging = ctxConf->debugLogging;
    conf->disableOptimizations = ctxConf->disableOptimizations;

    try {
        const char* tmp = nullptr;

        tmp = getStringArg(argc, argv, "s");
        if (tmp) conf->script_path = tmp;

        tmp = getStringArg(argc, argv, "c");
        if (tmp) conf->config_path = tmp;

        tmp = getStringArg(argc, argv, "o");
        if (tmp) {
            if (strcmp(tmp, "all") == 0) conf->mode = om_all;
            else if (strcmp(tmp, "ast") == 0) conf->mode = om_ast;
            else if (strcmp(tmp, "funcs") == 0) conf->mode = om_funcs;
            else if (strcmp(tmp, "types") == 0) conf->mode = om_types;
            else if (strcmp(tmp, "code") == 0) conf->mode = om_code;
            else if (strcmp(tmp, "logs") == 0) conf->mode = om_logs;
            else {
                printf("Invalid output option '%s'.\n", tmp);
                print_help();
                return ARGUMENT_ERROR;
            }
        }

        if (getBoolArg(argc, argv, "m")) conf->pretty_print = false;
        if (getBoolArg(argc, argv, "t")) conf->trusted = true;
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
    fopen_s(&fp, filename, "r");
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

i32 handleResult(Context* ctx, const script_metadata& meta, const tsnc_config& conf) {
    static const char* access[] = { "public", "private", "trusted" };
    const auto& logs = ctx->getPipeline()->getLogger()->getMessages();
    auto c = ctx->getPipeline()->getCompiler();
    bool hadErrors = ctx->getPipeline()->getLogger()->hasErrors();

    json out;
    switch (conf.mode) {
        case om_all: {
            out["logs"] = json::array();
            out["symbols"] = json::array();
            out["types"] = json::array();
            out["functions"] = json::array();
            out["globals"] = json::array();
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
            const auto& funcs = c->getOutput()->getFuncs();
            for (u32 i = 0;i < funcs.size();i++) {
                if (!funcs[i] || !funcs[i]->getOutput()) continue;
                json func = toJson(funcs[i]->getOutput(), true, funcs[i]);
                json& oc = func["code"] = json::array();

                const auto& code = funcs[i]->getCode();
                for (u32 c = 0;c < code.size();c++) {
                    oc.push_back(code[c].toString(funcs[i]->getContext()).c_str());
                }

                o.push_back(func);
            }
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