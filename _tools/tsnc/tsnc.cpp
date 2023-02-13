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

struct tsnc_config {
    const char* script_path;
    const char* config_path;
    bool pretty_print;
};

void print_help();
const char* getStringArg(i32 argc, const char** argv, const char* code);
bool getBoolArg(i32 argc, const char** argv, const char* code);
i32 parse_args(i32 argc, const char** argv, tsnc_config* conf);
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
    i32 status = parse_args(argc, argv, &conf);
    if (status != 0) {
        return status;
    }

    Config contextCfg;
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
    
    script_metadata meta;
    try {
        meta.path = std::filesystem::relative(conf.script_path, contextCfg.workspaceRoot.c_str()).string();
        meta.size = (size_t)std::filesystem::file_size(conf.script_path);
        meta.is_trusted = true;
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
    printf("    -s <script file>\tSpecifies entrypoint script to compile. Defaults to './main.tsn'.\n");
    printf("    -c <config file>\tSpecifies compiler configuration file, must be JSON format. Defaults to './tsnc.json'.\n");
    printf("    -m              \tOutputs minified json instead of pretty-printed json.\n");
    printf("    -h              \tPrints this information.\n");
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

i32 parse_args(i32 argc, const char** argv, tsnc_config* conf) {
    conf->script_path = "./main.tsn";
    conf->config_path = "./tsnc.json";
    conf->pretty_print = true;

    try {
        const char* tmp = nullptr;

        tmp = getStringArg(argc, argv, "s");
        if (tmp) conf->script_path = tmp;

        tmp = getStringArg(argc, argv, "c");
        if (tmp) conf->config_path = tmp;

        if (getBoolArg(argc, argv, "m")) conf->pretty_print = false;

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
    json dirs = configIn.at("directories");
    if (!dirs.empty()) {
        json wsroot = dirs.at("root");
        if (!wsroot.empty()) {
            if (!wsroot.is_string()) {
                initError("config.directories.root is not a string. It should be the path to the root of the script workspace", tsncConf);
                return CONFIG_VALUE_ERROR;
            }
            configOut.workspaceRoot = wsroot;
        }

        json supportDir = dirs.at("support");
        if (!supportDir.empty()) {
            if (!supportDir.is_string()) {
                initError("config.directories.support is not a string. It should be the path to the folder where the compiler will store persistence data", tsncConf);
                return CONFIG_VALUE_ERROR;
            }
            configOut.supportDir = supportDir;
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
    out["logs"] = json::array();
    out["symbols"] = json::array();
    out["types"] = json::array();
    out["functions"] = json::array();
    out["globals"] = json::array();
    out["ast"] = json(nullptr);

    for (u32 i = 0;i < logs.size();i++) {
        out["logs"].push_back(toJson(logs[i]));
    }

    if (c && c->getOutput()) {
        const auto& syms = c->getOutput()->getSymbolLifetimeData();
        for (u32 i = 0;i < syms.size();i++) {
            if (syms[i].sym->getFlags().is_type && syms[i].sym->getType()->getInfo().is_host) continue;
            if (syms[i].sym->getFlags().is_type && syms[i].sym->getType()->getInfo().is_function) continue;
            if (syms[i].sym->getFlags().is_function && syms[i].sym->isImm()) continue;
            if (syms[i].name.firstIndexOf('@') != -1) continue;
            out["symbols"].push_back(toJson(syms[i]));
        }

        const auto& types = c->getOutput()->getTypes();
        for (u32 i = 0;i < types.size();i++) {
            out["types"].push_back(toJson(types[i], false));
        }

        const auto& funcs = c->getOutput()->getFuncs();
        for (u32 i = 0;i < funcs.size();i++) {
            if (!funcs[i] || !funcs[i]->getOutput()) continue;
            out["functions"].push_back(toJson(funcs[i]->getOutput(), false, funcs[i]));
        }

        const auto& globals = c->getOutput()->getModule()->getData();
        for (u32 i = 0;i < globals.size();i++) {
            out["globals"].push_back(toJson(i, globals[i]));
        }
    }

    if (ctx->getPipeline()->getAST()) out["ast"] = toJson(ctx->getPipeline()->getAST());

    if (conf.pretty_print) printf(out.dump(4).c_str());
    else printf(out.dump().c_str());

    if (hadErrors) return COMPILATION_ERROR;
    return COMPILATION_SUCCESS;
}