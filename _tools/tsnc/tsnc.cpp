#include <tsn/utils/ProgramSource.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/common/Module.h>
#include <tsn/common/DataType.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/Function.h>
#include <tsn/interfaces/IDataTypeHolder.h>
#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/compiler/Lexer.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/Output.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/FunctionDef.h>

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

struct tsnc_config {
    const char* script_path;
    const char* config_path;
};

void print_help();
const char* getStringArg(i32 argc, const char** argv, const char* code);
bool getBoolArg(i32 argc, const char** argv, const char* code);
i32 parse_args(i32 argc, const char** argv, tsnc_config* conf);
char* loadText(const char* filename, bool required);
i32 processConfig(const json& configIn, Config& configOut);
i32 handleAST(Context* ctx, ParseNode* n, const Parser& ps);
void initError(const char* err);

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

    char* code = nullptr;
    try {
        code = loadText(conf.script_path, true);
    } catch (i32 error) {
        return error;
    }

    Config contextCfg;
    try {
        char* configInput = loadText(conf.config_path, false);
        if (configInput) {
            json config = json::parse(configInput);
            delete [] configInput;

            status = processConfig(config, contextCfg);
            if (status != 0) {
                delete [] code;
                return status;
            }
        }
    } catch (i32 error) {
        delete [] code;
        return error;
    } catch (std::exception exc) {
        initError(utils::String::Format("Encountered exception while parsing config: %s", exc.what()).c_str());
        delete [] code;
        return CONFIG_PARSE_ERROR;
    }

    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    {
        Context ctx = Context(1, &contextCfg);
        ProgramSource src = ProgramSource(conf.script_path, code);
        Lexer l(&src);
        Parser ps(&l);
        ParseNode* n = ps.parse();

        if (n) status = handleAST(&ctx, n, ps);
        else {
            initError("An unknown error occurred");
            status = UNKNOWN_ERROR;
        }
    }

    utils::Mem::Destroy();
    utils::String::Allocator::Destroy();

    delete [] code;
    return status;
}

void print_help() {
    printf("TSN Compiler version 0.0.0\n");
    printf("Info:\n");
    printf("    This tool is used to compile and/or validate TSN scripts. It will attempt to\n");
    printf("    compile the provided script, and subsequently output metadata about the script\n");
    printf("    in the JSON format. This metadata includes the abstract syntax tree, symbols,\n");
    printf("    log messages, data types, and IR code.\n");
    printf("Usage:\n");
    printf("    -s <script file>\tSpecifies entrypoint script to compile. Defaults to './main.tsn'.\n");
    printf("    -c <config file>\tSpecifies compiler configuration file, must be JSON format. Defaults to './tsnc.json'.\n");
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

    try {
        const char* tmp = nullptr;

        tmp = getStringArg(argc, argv, "s");
        if (tmp) conf->script_path = tmp;

        tmp = getStringArg(argc, argv, "c");
        if (tmp) conf->config_path = tmp;

        if (getBoolArg(argc, argv, "h")) {
            print_help();
            return EXIT_EARLY;
        }
    } catch(i32 code) {
        return code;
    }

    return 0;
}

char* loadText(const char* filename, bool required) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "r");
    if (!fp) {
        if (!required) return nullptr;

        initError(utils::String::Format("Unable to open file '%s'", filename).c_str());
        throw FAILED_TO_OPEN_FILE;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz == 0) {
        initError(utils::String::Format("File '%s' is empty", filename).c_str());
        fclose(fp);
        throw FILE_EMPTY;
    }

    char* code = new char[sz + 1];
    if (!code) {
        initError(utils::String::Format("Failed to allocate input buffer for '%s'", filename).c_str());
        fclose(fp);
        throw FAILED_TO_ALLOCATE_INPUT_BUFFER;
    }

    size_t readSz = fread(code, 1, sz, fp);
    if (readSz != sz && !feof(fp)) {
        delete [] code;
        initError(utils::String::Format("Failed to read '%s'", filename).c_str());
        fclose(fp);
        throw FAILED_TO_READ_FILE;
    }
    code[readSz] = 0;
    
    fclose(fp);

    return code;
}

i32 processConfig(const json& configIn, Config& configOut) {
    json dirs = configIn.at("directories");
    if (!dirs.empty()) {
        json wsroot = dirs.at("root");
        if (!wsroot.empty()) {
            if (!wsroot.is_string()) {
                initError("config.directories.root is not a string. It should be the path to the root of the script workspace");
                return CONFIG_VALUE_ERROR;
            }
            configOut.workspaceRoot = wsroot;
        }

        json supportDir = dirs.at("support");
        if (!supportDir.empty()) {
            if (!supportDir.is_string()) {
                initError("config.directories.support is not a string. It should be the path to the folder where the compiler will store persistence data");
                return CONFIG_VALUE_ERROR;
            }
            configOut.supportDir = supportDir;
        }
    }

    return 0;
}

void initError(const char* err) {
    printf("{\n");
    printf("    \"logs\": [\n");
    printf("        {\n");
    printf("            \"code\": null,\n");
    printf("            \"type\": \"error\",\n");
    printf("            \"range\": null,\n");
    printf("            \"line_txt\": null,\n");
    printf("            \"line_idx\": null,\n");
    printf("            \"message\": \"%s\",\n", err);
    printf("            \"ast\": null\n");
    printf("        }\n");
    printf("    ],\n");
    printf("    \"symbols\": [],\n");
    printf("    \"types\": [],\n");
    printf("    \"functions\": [],\n");
    printf("    \"globals\": [],\n");
    printf("    \"ast\": null\n");
    printf("}");
}

i32 handleAST(Context* ctx, ParseNode* n, const Parser& ps) {
    static const char* access[] = { "public", "private", "trusted" };
    const auto& errors = ps.errors();
    if (errors.size() == 0) {
        Compiler c(ctx, n);
        CompilerOutput* out = c.compile();

        Buffer* test = new Buffer();
        if (out->serialize(test, ctx)) {
            test->save("./out.tsnc");
            delete test;
        }

        if (out) {
            bool hadErrors = false;
            printf("{\n");

            const auto& logs = c.getLogs();
            if (logs.size() > 0) {
                const char* logTps[] = { "info", "warning", "error" };
                printf("    \"logs\": [\n");
                for (u32 i = 0;i < logs.size();i++) {
                    const auto& log = logs[i];
                    if (log.type == cmt_error) hadErrors = true;
                    const SourceLocation& src = log.src;
                    const SourceLocation& end = log.src.getEndLocation();
                    String ln = src.getSource()->getLine(src.getLine()).clone();
                    ln.replaceAll("\n", "");
                    ln.replaceAll("\r", "");
                    u32 wsc = 0;
                    for (;wsc < ln.size();wsc++) {
                        if (isspace(ln[wsc])) continue;
                        break;
                    }

                    u32 baseOffset = src.getOffset();
                    u32 endOffset = end.getOffset();

                    utils::String indxStr = "";
                    for (u32 i = 0;i < (src.getCol() - wsc);i++) indxStr += ' ';
                    for (u32 i = 0;i < (endOffset - baseOffset) && indxStr.size() < (ln.size() - wsc);i++) {
                        indxStr += '^';
                    }

                    printf("        {\n");
                    printf("            \"code\": \"C%d\",\n", log.code);
                    printf("            \"type\": \"%s\",\n", logTps[log.type]);
                    printf("            \"range\": {\n");
                    printf("                \"start\": {\n");
                    printf("                    \"line\": %d,\n", src.getLine());
                    printf("                    \"col\": %d,\n", src.getCol());
                    printf("                    \"offset\": %d\n", baseOffset);
                    printf("                },\n");
                    printf("                \"end\": {\n");
                    printf("                    \"line\": %d,\n", end.getLine());
                    printf("                    \"col\": %d,\n", end.getCol());
                    printf("                    \"offset\": %d\n", endOffset);
                    printf("                }\n");
                    printf("            },\n");
                    printf("            \"line_txt\": \"%s\",\n", ln.c_str() + wsc);
                    printf("            \"line_idx\": \"%s\",\n", indxStr.c_str());
                    printf("            \"message\": \"%s\",\n", log.msg.c_str());
                    printf("            \"ast\": ");
                    log.node->json(3);
                    printf("\n");
                    printf("        }%s\n", (i == logs.size() - 1) ? "" : ",");
                }
                printf("    ],\n");
            } else printf("    \"logs\": [],\n");

            const auto& syms = c.getOutput()->getSymbolLifetimeData();
            if (syms.size() > 0) {
                printf("    \"symbols\": [\n");
                for (u32 i = 0;i < syms.size();i++) {
                    const auto& s = syms[i];
                    const char* type = "value";

                    String detail = "";
                    if (s.sym->getFlags().is_function) {
                        type = "function";
                        FunctionDef* fn = s.sym->getImm<FunctionDef*>();
                        if (fn->getOutput()) detail = fn->getOutput()->getFullyQualifiedName();
                        else detail = fn->getName();
                    } else if (s.sym->getFlags().is_module) {
                        type = "module";
                    } else if (s.sym->getFlags().is_type) {
                        type = "type";
                        detail = s.sym->getType()->getFullyQualifiedName();
                    } else {
                        detail = s.sym->getType()->getFullyQualifiedName();
                    }

                    printf("        {\n");
                    printf("            \"name\": \"%s\",\n", s.name.c_str());
                    printf("            \"type\": \"%s\",\n", type);
                    printf("            \"detail\": \"%s\",\n", detail.c_str());
                    printf("            \"range\": {\n");
                    printf("                \"start\": {\n");
                    printf("                    \"line\": %d,\n", s.begin.getLine());
                    printf("                    \"col\": %d,\n", s.begin.getCol());
                    printf("                    \"offset\": %d\n", s.begin.getOffset());
                    printf("                },\n");
                    printf("                \"end\": {\n");
                    printf("                    \"line\": %d,\n", s.end.getLine());
                    printf("                    \"col\": %d,\n", s.end.getCol());
                    printf("                    \"offset\": %d\n", s.end.getOffset());
                    printf("                }\n");
                    printf("            }\n");
                    printf("        }%s\n", (i == syms.size() - 1) ? "" : ",");
                }
                printf("    ],\n");
            } else printf("    \"symbols\": [],\n");

            const auto& types = c.getOutput()->getTypes();
            if (types.size() > 0) {
                printf("    \"types\": [\n");
                for (u32 i = 0;i < types.size();i++) {
                    ffi::DataType* t = types[i];
                    const type_meta& info = t->getInfo();
                    printf("        {\n");
                    printf("            \"id\": %d,\n", t->getId());
                    printf("            \"size\": %u,\n", info.size);
                    printf("            \"host_hash\": %zu,\n", info.host_hash);
                    printf("            \"name\": \"%s\",\n", t->getName().c_str());
                    printf("            \"access\": \"%s\",\n", access[t->getAccessModifier()]);
                    printf("            \"fully_qualified_name\": \"%s\",\n", t->getFullyQualifiedName().c_str());
                    printf("            \"destructor\": ");
                    if (t->getDestructor()) {
                        printf("{\n");
                        printf("                \"name\": \"%s\",\n", t->getDestructor()->getDisplayName().c_str());
                        printf("                \"fully_qualified_name\": \"%s\",\n", t->getDestructor()->getFullyQualifiedName().c_str());
                        printf("                \"access\": \"%s\"\n", access[t->getDestructor()->getAccessModifier()]);
                        printf("            },\n");
                    } else printf("null,\n");
                    printf("            \"flags\": [\n");
                    u8 f = 0;
                    if (info.is_pod                    ) { printf("                \"is_pod\""); f++; }
                    if (info.is_trivially_constructible) { printf("%s\"is_trivially_constructible\"", (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_trivially_copyable     ) { printf("%s\"is_trivially_copyable\"",      (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_trivially_destructible ) { printf("%s\"is_trivially_destructible\"",  (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_primitive              ) { printf("%s\"is_primitive\"",               (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_floating_point         ) { printf("%s\"is_floating_point\"",          (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_integral               ) { printf("%s\"is_integral\"",                (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_unsigned               ) { printf("%s\"is_unsigned\"",                (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_function               ) { printf("%s\"is_function\"",                (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_template               ) { printf("%s\"is_template\"",                (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_alias                  ) { printf("%s\"is_alias\"",                   (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_host                   ) { printf("%s\"is_host\"",                    (f > 0) ? ",\n                " : "                "); f++; }
                    if (info.is_anonymous              ) { printf("%s\"is_anonymous\"",               (f > 0) ? ",\n                " : "                "); f++; }
                    printf("\n            ],\n");

                    if (info.is_alias) {
                        ffi::AliasType* a = (ffi::AliasType*)t;
                        printf("            \"alias_of\": \"%s\",\n", a->getRefType()->getFullyQualifiedName().c_str());
                    } else {
                        printf("            \"alias_of\": null,\n");
                    }

                    const auto& bases = t->getBases();
                    if (bases.size() > 0) {
                        printf("            \"inherits\": [\n");
                        for (u32 b = 0;b < bases.size();b++) {
                            const ffi::type_base& base = bases[b];
                            printf("                {\n");
                            printf("                    \"type\": \"%s\",\n", base.type->getFullyQualifiedName().c_str());
                            printf("                    \"access\": \"%s\",\n", access[base.access]);
                            printf("                    \"data_offset\": %llu\n", base.offset);
                            printf("                }%s\n", (b == bases.size() - 1) ? "" : ",");
                        }
                        printf("             ],\n");
                    } else printf("            \"inherits\": [],\n");

                    const auto& props = t->getProperties();
                    if (props.size() > 0) {
                        printf("            \"properties\": [\n");
                        for (u32 p = 0;p < props.size();p++) {
                            const ffi::type_property& prop = props[p];
                            printf("                {\n");
                            printf("                    \"name\": \"%s\",\n", prop.name.c_str());
                            printf("                    \"type\": \"%s\",\n", prop.type->getFullyQualifiedName().c_str());
                            printf("                    \"offset\": %llu,\n", prop.offset);
                            printf("                    \"access\": \"%s\",\n", access[prop.access]);
                            printf("                    \"getter\": %s,\n", prop.getter ? ("\"" + prop.getter->getName() + "\"").c_str() : "null");
                            printf("                    \"setter\": %s,\n", prop.setter ? ("\"" + prop.setter->getName() + "\"").c_str() : "null");
                            printf("                    \"flags\": [");
                            u8 f = 0;
                            if (prop.flags.is_static) { printf("\"is_static\""); f++; }
                            if (prop.flags.is_pointer) { printf("%s\"is_pointer\"", (f > 0) ? ", " : ""); f++; }
                            if (prop.flags.can_read) { printf("%s\"can_read\"", (f > 0) ? ", " : ""); f++; }
                            if (prop.flags.can_write) { printf("%s\"can_write\"", (f > 0) ? ", " : ""); f++; }
                            printf("]\n");
                            printf("                }%s\n", (p == props.size() - 1) ? "" : ",");
                        }
                        printf("             ],\n");
                    } else printf("            \"properties\": [],\n");

                    const auto& methods = t->getMethods();
                    if (methods.size() > 0) {
                        printf("            \"methods\": [\n");
                        for (u32 m = 0;m < methods.size();m++) {
                            const ffi::Function* method = methods[m];
                            printf("                {\n");
                            printf("                    \"name\": \"%s\",\n", method->getName().c_str());
                            printf("                    \"fully_qualified_name\": \"%s\",\n", method->getFullyQualifiedName().c_str());
                            printf("                    \"access\": \"%s\"\n", access[method->getAccessModifier()]);
                            printf("                }%s\n", (m == methods.size() - 1) ? "" : ",");
                        }
                        printf("             ],\n");
                    } else printf("            \"methods\": [],\n");

                    if (info.is_template) {
                        printf("            \"ast\": ");
                        ffi::TemplateType* tt = (ffi::TemplateType*)t;
                        tt->getAST()->json(3);
                        printf("\n");
                    } else printf("            \"ast\": null\n");
                    
                    printf("        }%s\n", (i == types.size() - 1) ? "" : ",");
                }
                printf("    ],\n");
            } else printf("    \"types\": [],\n");

            const auto& funcs = c.getOutput()->getFuncs();
            if (funcs.size() > 0) {
                printf("    \"functions\": [\n");
                for (u32 i = 0;i < funcs.size();i++) {
                    ffi::Function* f = funcs[i]->getOutput();
                    if (!f) continue;
                    ffi::FunctionType* sig = f->getSignature();
                    
                    printf("        {\n");
                    printf("            \"id\": %d,\n", f->getId());
                    printf("            \"name\": \"%s\",\n", f->getName().c_str());
                    printf("            \"fully_qualified_name\": \"%s\",\n", f->getFullyQualifiedName().c_str());
                    if (f->isTemplate()) {
                        printf("            \"signature\": null,\n");
                    } else {
                        printf("            \"signature\": \"%s\",\n", sig->getFullyQualifiedName().c_str());
                    }
                    printf("            \"access\": \"%s\",\n", access[f->getAccessModifier()]);
                    printf("            \"is_method\": %s,\n", f->isMethod() ? "true" : "false");
                    if (f->isTemplate()) {
                        printf("            \"is_thiscall\": null,\n");
                    } else {
                        printf("            \"is_thiscall\": %s,\n", f->isThisCall() ? "true" : "false");
                    }
                    printf("            \"is_template\": %s,\n", f->isTemplate() ? "true" : "false");
                    
                    if (f->isTemplate()) {
                        printf("            \"args\": [],\n");
                        printf("            \"code\": [],\n");
                        printf("            \"ast\": ");
                        ParseNode* ast = nullptr;
                        if (f->isMethod()) ast = ((ffi::TemplateMethod*)f)->getAST();
                        else ast = ((ffi::TemplateFunction*)f)->getAST();
                        ast->json(3);
                        printf("\n");
                    } else {
                        const auto& args = sig->getArguments();
                        if (args.size() > 0) {
                            printf("            \"args\": [\n");

                            static const char* argTpStr[] = {
                                "func_ptr",
                                "ret_ptr",
                                "ectx_ptr",
                                "this_ptr",
                                "value",
                                "pointer"
                            };
                            
                            for (u8 a = 0;a < args.size();a++) {
                                const auto& arg = args[a];
                                utils::String loc;
                                if (arg.argType == arg_type::context_ptr) {
                                    loc = utils::String::Format("\"%s\"", funcs[i]->getECtx().toString().c_str());
                                } else if (arg.argType == arg_type::func_ptr) {
                                    loc = utils::String::Format("\"%s\"", funcs[i]->getFPtr().toString().c_str());
                                } else if (arg.argType == arg_type::ret_ptr) {
                                    if (funcs[i]->getReturnType()->isEqualTo(c.getContext()->getTypes()->getType<void>())) loc = "null";
                                    else loc = utils::String::Format("\"%s\"", funcs[i]->getRetPtr().toString().c_str());
                                } else if (arg.argType == arg_type::this_ptr) {
                                    if (funcs[i]->getThisType()->isEqualTo(c.getContext()->getTypes()->getType<void>())) loc = "null";
                                    else loc = utils::String::Format("\"%s\"", funcs[i]->getThis().toString().c_str());
                                } else {
                                    loc = utils::String::Format("\"%s\"", funcs[i]->getArg(a - funcs[i]->getImplicitArgCount()).toString().c_str());
                                }

                                printf("                {\n");
                                printf("                    \"arg_type\": \"%s\",\n", argTpStr[u32(arg.argType)]);
                                printf("                    \"is_implicit\": %s,\n", arg.isImplicit() ? "true" : "false");
                                printf("                    \"data_type\": \"%s\",\n", arg.dataType->getFullyQualifiedName().c_str());
                                printf("                    \"location\": %s\n", loc.c_str());
                                printf("                }%s\n", (a == args.size() - 1) ? "" : ",");
                            }
                            printf("            ],\n");
                        } else printf("            \"args\": [],\n");

                        const auto& code = funcs[i]->getCode();
                        if (code.size() > 0) {
                            printf("            \"code\": [\n");
                            for (u32 c = 0;c < code.size();c++) {
                                printf("                \"%s\"%s\n", code[c].toString().c_str(), c < code.size() - 1 ? "," : "");
                            }
                            printf("            ],\n");
                        } else printf("            \"code\": [],\n");
                        printf("            \"ast\": null\n");
                    }
                    
                    printf("        }%s\n", (i == funcs.size() - 1) ? "" : ",");
                }
                printf("    ],\n");
            } else printf("    \"functions\": [],\n");

            const auto& globals = c.getOutput()->getModule()->getData();
            if (globals.size() > 0) {
                printf("    \"globals\": [\n");
                for (u32 i = 0;i < globals.size();i++) {
                    const auto& var = globals[i];
                    printf("        {\n");
                    printf("            \"name\": \"%s\",\n", var.name.c_str());
                    printf("            \"access\": \"%s\",\n", access[var.access]);
                    printf("            \"type\": \"%s\"\n", var.type->getFullyQualifiedName().c_str());
                    printf("        }%s\n", (i == globals.size() - 1) ? "" : ",");
                }
                printf("    ],\n");
            } else printf("    \"globals\": [],\n");

            printf("    \"ast\": ");
            n->json(1);
            printf("\n}\n");
            delete out;

            if (hadErrors) return COMPILATION_ERROR;
        }
    } else {
        printf("{\n");
        printf("    \"logs\": [\n");
        for (u32 i = 0;i < errors.size();i++) {
            const auto& e = errors[i];
            const SourceLocation& src = e.src.src;
            const SourceLocation& end = e.src.src.getEndLocation();
            String ln = src.getSource()->getLine(src.getLine()).clone();
            ln.replaceAll("\n", "");
            ln.replaceAll("\r", "");
            u32 wsc = 0;
            for (;wsc < ln.size();wsc++) {
                if (isspace(ln[wsc])) continue;
                break;
            }

            u32 baseOffset = src.getOffset();
            u32 endOffset = end.getOffset();

            utils::String indxStr = "";
            for (u32 i = 0;i < (src.getCol() - wsc);i++) indxStr += ' ';
            for (u32 i = 0;i < (endOffset - baseOffset) && indxStr.size() < (ln.size() - wsc);i++) {
                indxStr += '^';
            }

            printf("        {\n");
            printf("            \"code\": \"P%d\",\n", e.code);
            printf("            \"type\": \"error\",\n");
            printf("            \"range\": {\n");
            printf("                \"start\": {\n");
            printf("                    \"line\": %d,\n", src.getLine());
            printf("                    \"col\": %d,\n", src.getCol());
            printf("                    \"offset\": %d\n", baseOffset);
            printf("                },\n");
            printf("                \"end\": {\n");
            printf("                    \"line\": %d,\n", end.getLine());
            printf("                    \"col\": %d,\n", end.getCol());
            printf("                    \"offset\": %d\n", endOffset);
            printf("                }\n");
            printf("            },\n");
            printf("            \"line_txt\": \"%s\",\n", ln.c_str() + wsc);
            printf("            \"line_idx\": \"%s\",\n", indxStr.c_str());
            printf("            \"message\": \"%s\",\n", e.text.c_str());
            printf("            \"ast\": null\n");
            printf("        }%s\n", (i == errors.size() - 1) ? "" : ",");
        }
        printf("    ],\n");
        printf("    \"symbols\": [],\n");
        printf("    \"types\": [],\n");
        printf("    \"functions\": [],\n");
        printf("    \"globals\": [],\n");
        printf("    \"ast\": ");
        n->json(1);
        printf("\n}\n");

        if (errors.size() > 0) return COMPILATION_ERROR;
    }

    return COMPILATION_SUCCESS;
}