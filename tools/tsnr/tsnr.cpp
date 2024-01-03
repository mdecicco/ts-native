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
#include <tsn/utils/SourceLocation.h>
#include <tsn/utils/ModuleSource.h>

#include <utils/Timer.h>
#include <utils/Array.hpp>
#include <utils/Buffer.hpp>
#include <utils/json.hpp>


#include <filesystem>

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

enum backend_type {
    bt_none,
    bt_vm
};

struct tsnc_config {
    const char* script_path;
    const char* config_path;
    const char* entry_point;
    backend_type backend;
    bool trusted;
    bool disableOptimizations;
    bool logDuration;
};

void print_help();
const char* getStringArg(i32 argc, const char** argv, const char* code);
bool getBoolArg(i32 argc, const char** argv, const char* code);
i32 parse_args(i32 argc, const char** argv, tsnc_config* conf, Config* ctxConf);
char* loadText(const char* filename, bool required, const tsnc_config& conf);
i32 processConfig(const json& configIn, Config& configOut, const tsnc_config& tsncConf);
i32 handleResult(Context* ctx, Module* mod, const script_metadata& meta, const tsnc_config& conf);
String formatDuration(f32 seconds);

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
        printf("Encountered exception while parsing config: %s\n", exc.what());
        return CONFIG_PARSE_ERROR;
    }

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
        printf("Encountered exception while getting script metadata: %s\n", exc.what());
        return UNKNOWN_ERROR;
    }

    if (meta.size == 0) {
        printf("The specified script is empty");
        return FILE_EMPTY;
    }

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

        f32 compilationTime, executionTime;

        utils::Timer tmr;
        tmr.start();
        Module* mod = ctx.getPipeline()->buildFromSource(&meta);
        compilationTime = tmr;
        tmr.reset();

        
        tmr.start();
        if (mod && be) mod->init();
        executionTime = tmr;
        tmr.reset();

        status = handleResult(&ctx, mod, meta, conf);

        if (conf.logDuration) {
            printf("Compilation time: %s\n", formatDuration(compilationTime).c_str());
            if (mod && be && !ctx.getPipeline()->getLogger()->hasErrors()) {
                printf("Execution time: %s\n", formatDuration(executionTime).c_str());
            }
        }

        ctx.shutdown();
        if (be) delete be;
    }

    tsn::ffi::ExecutionContext::Shutdown();
    utils::Mem::Destroy();

    return status;
}

void print_help() {
    printf("TSN Executor version 0.0.1\n");
    printf("Info:\n");
    printf("    This tool is used to compile execute TSN scripts.\n");
    printf("Usage:\n");
    printf("    -s <script file>    Specifies entrypoint script to compile. Defaults to './main.tsn'.\n");
    printf("    -c <config file>    Specifies compiler configuration file, must be JSON format. Defaults to './tsnc.json'.\n");
    printf("    -t                  Compiles the specified script in trusted mode\n");
    printf("    -u                  Disables optimization (overrides configuration file)\n");
    printf("    -p                  Logs timing information\n");
    printf("    -b <backend>        Specifies the backend to use for running compiled code.\n");
    printf("                            none     Does not run code\n");
    printf("                            vm       Uses a register-based VM to run code (default)\n");
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
    conf->trusted = false;
    conf->disableOptimizations = ctxConf->disableOptimizations;
    conf->logDuration = false;
    conf->backend = bt_vm;

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

        if (getBoolArg(argc, argv, "t")) conf->trusted = true;
        if (getBoolArg(argc, argv, "p")) conf->logDuration = true;
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

        printf("Unable to open file '%s'", filename);
        throw FAILED_TO_OPEN_FILE;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz == 0) {
        printf("File '%s' is empty", filename);
        fclose(fp);
        throw FILE_EMPTY;
    }

    char* code = new char[sz + 1];
    if (!code) {
        printf("Failed to allocate input buffer for '%s'", filename);
        fclose(fp);
        throw FAILED_TO_ALLOCATE_INPUT_BUFFER;
    }

    size_t readSz = fread(code, 1, sz, fp);
    if (readSz != sz && !feof(fp)) {
        delete [] code;
        printf("Failed to read '%s'", filename);
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
                        printf("config.directories.root is not a string. It should be the path to the root of the script workspace");
                        return CONFIG_VALUE_ERROR;
                    }
                    configOut.workspaceRoot = wsroot;
                }
            }

            if (dirs.contains("support")) {
                json supportDir = dirs["support"];
                if (!supportDir.empty()) {
                    if (!supportDir.is_string()) {
                        printf("config.directories.support is not a string. It should be the path to the folder where the compiler will store persistence data");
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
                printf("config.enableDebugLogs is not a boolean.");
                return CONFIG_VALUE_ERROR;
            }

            configOut.debugLogging = debug;
        }
    }

    if (configIn.contains("disableOptimizations")) {
        json disableOpt = configIn["disableOptimizations"];
        if (!disableOpt.empty()) {
            if (!disableOpt.is_boolean()) {
                printf("config.disableOptimizations is not a boolean.");
                return CONFIG_VALUE_ERROR;
            }

            configOut.disableOptimizations = disableOpt;
        }
    }

    return 0;
}

i32 handleResult(Context* ctx, Module* mod, const script_metadata& meta, const tsnc_config& conf) {
    bool hadErrors = ctx->getPipeline()->getLogger()->hasErrors();

    const auto& logs = ctx->getPipeline()->getLogger()->getMessages();
    constexpr const char* logTps[] = { "debug", "info", "warning", "error" };

    for (u32 i = 0;i < logs.size();i++) {
        if (i != 0) printf("\n");

        const log_message& log = logs[i];

        char prefix = 'U';
        if (log.code < IO_MESSAGES_START) prefix = 'G';
        else if (log.code > IO_MESSAGES_START && log.code < IO_MESSAGES_END) prefix = 'I';
        else if (log.code > PARSE_MESSAGES_START && log.code < PARSE_MESSAGES_END) prefix = 'P';
        else if (log.code > COMPILER_MESSAGES_START && log.code < COMPILER_MESSAGES_END) prefix = 'C';

        ModuleSource* ms = log.src.getSource();
        if (ms) printf("%s (line %d, col %d)\n", ms->getMeta()->path.c_str(), log.src.getLine() + 1, log.src.getCol());
        printf("[%c%d %s] %s\n", prefix, log.code, logTps[log.type], log.msg.c_str());

        if (ms) {
            SourceLocation end = log.src.getEndLocation();
            u32 baseOffset = log.src.getOffset();
            u32 endOffset = end.getOffset();

            u32 wsc = UINT32_MAX;
            String indxStr = "";
            Array<String> lines;
            Array<u32> lineNums;
            u32 lineNumWidth = 1;

            for (i32 i = i32(log.src.getLine()) - 3;i < i32(log.src.getLine()) + 3 && i < i32(ms->getLineCount());i++) {
                if (i < 0) continue;

                String ln = ms->getLine(i).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");

                lines.push(ln);
                lineNums.push(i + 1);

                u32 digitCount = 0;
                u32 x = i + 1;
                while (x) {
                    x /= 10;
                    digitCount++;
                }

                if (digitCount > lineNumWidth) lineNumWidth = digitCount;

                u32 wc = 0;
                for (;wc < ln.size();wc++) {
                    if (isspace(ln[wc])) continue;
                    break;
                }

                if (wc < wsc) wsc = wc;
            }
            
            String errLn = ms->getLine(log.src.getLine()).clone();
            errLn.replaceAll("\n", "");
            errLn.replaceAll("\r", "");

            for (u32 i = 0;i < lineNumWidth;i++) indxStr += ' ';
            indxStr += " | ";
            u32 b = log.src.getCol() - wsc;
            u32 e = end.getCol() - wsc;
            for (u32 i = 0;i < e && i < errLn.size() - wsc;i++) {
                if (i >= b) indxStr += '^';
                else indxStr += ' ';
            }

            char lnFmt[16] = { 0 };
            snprintf(lnFmt, 16, "%%%dd | %%s\n", lineNumWidth);

            for (u32 i = 0;i < lines.size();i++) {
                u32 lineNum = lineNums[i];
                printf(lnFmt, lineNum, lines[i].c_str() + wsc);
                if (lineNum == log.src.getLine() + 1) printf("%s\n", indxStr.c_str());
            }
        }
    }

    if (hadErrors) return COMPILATION_ERROR;
    return COMPILATION_SUCCESS;
}

String formatDuration(f32 seconds) {
    String out;

    f64 t = seconds;
    u32 hours = u32(floor(t * 0.00027777777));
    t -= f64(hours) * 3600.0;

    u32 minutes = u32(floor(t * 0.01666666666));
    t -= f64(minutes) * 60.0;

    if (hours) {
        if (out.size() > 0) out += " ";
        out += String::Format("%d hour%s", hours, hours != 1 ? "s" : "");
    }

    if (minutes) {
        if (out.size() > 0) out += " ";
        out += String::Format("%d minute%s", minutes, minutes != 1 ? "s" : "");
    }

    if (out.size() > 0) out += " ";
    out += String::Format("%0.4f second%s", seconds, seconds != 1.0 ? "s" : "");

    return out;
}