#include <gs/utils/ProgramSource.h>
#include <gs/common/Context.h>
#include <gs/common/Module.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/Function.h>
#include <gs/interfaces/IDataTypeHolder.h>
#include <gs/interfaces/IFunctionHolder.h>
#include <gs/compiler/Lexer.h>
#include <gs/compiler/Parser.h>
#include <gs/compiler/Compiler.h>
#include <gs/compiler/FunctionDef.h>

#include <utils/Array.hpp>

using namespace gs;
using namespace compiler;
using namespace utils;

void handleAST(Context* ctx, ast_node* n);

i32 main (i32 argc, const char** argv) {
    argc = 2;
    const char* a[] = { "", "./test.gs" };
    argv = a;

    if (argc != 2) {
        printf("Invalid usage. Correct usage is 'gs2json <path to gs file>'\n");
        return -2;
    }

    FILE* fp = nullptr;
    fopen_s(&fp, argv[1], "r");
    if (!fp) {
        printf("Unable to open input file\n");
        return -3;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz == 0) {
        printf("Input file is empty\n");
        fclose(fp);
        return -4;
    }

    char* code = new char[sz + 1];
    if (!code) {
        printf("Failed to allocate input buffer\n");
        fclose(fp);
        return -5;
    }

    size_t readSz = fread(code, 1, sz, fp);
    if (readSz != sz && !feof(fp)) {
        delete [] code;
        printf("Failed to read input file\n");
        fclose(fp);
        return -6;
    }
    code[readSz] = 0;
    
    fclose(fp);


    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    bool success = true;
    {
        Context ctx;
        ProgramSource src = ProgramSource(argv[1], code);
        Lexer l(&src);
        Parser ps(&l);
        ast_node* n = ps.parse();

        const auto& errors = ps.errors();
        if (errors.size() > 0) {
            success = false;

            printf("{\n");
            printf("    logs: [\n");
            for (u32 i = 0;i < errors.size();i++) {
                const auto& e = errors[i];
                const SourceLocation& src = e.src.src;
                String ln = src.getSource()->getLine(src.getLine()).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");
                u32 wsc = 0;
                for (;wsc < ln.size();wsc++) {
                    if (isspace(ln[wsc])) continue;
                    break;
                }

                printf("        {\n");
                printf("            \"code\": \"P%d\",\n", e.code);
                printf("            \"type\": \"error\",\n");
                printf("            \"line\": %d,\n", src.getLine());
                printf("            \"col\": %d,\n", src.getCol());
                printf("            \"line_txt\": \"%s\",\n", ln.c_str() + wsc);

                printf("            \"line_idx\": \"");
                for (u32 i = 0;i < (src.getCol() - wsc);i++) printf(" ");
                printf("^\",\n");

                printf("            \"message\": \"%s\",\n", e.text.c_str());
                printf("            \"ast\": null\n");
                printf("        }%s\n", (i == errors.size() - 1) ? "" : ",");
            }
            printf("    ],\n");
            printf("    \"types\": [],\n");
            printf("    \"functions\": [],\n");
            printf("    \"globals\": [],\n");
            printf("    \"ast\": null\n");
            printf("}");
        } else {
            if (n) handleAST(&ctx, n);
            else {
                printf("{\n");
                printf("    logs: [\n");
                printf("        {\n");
                printf("            \"code\": null,\n");
                printf("            \"type\": \"error\",\n");
                printf("            \"line\": null,\n");
                printf("            \"col\": null,\n");
                printf("            \"line_txt\": null,\n");
                printf("            \"line_idx\": null,");
                printf("            \"message\": \"An unknown error occurred\",\n");
                printf("            \"ast\": null\n");
                printf("        }\n");
                printf("    ],\n");
                printf("    \"types\": [],\n");
                printf("    \"functions\": [],\n");
                printf("    \"globals\": [],\n");
                printf("    \"ast\": null\n");
                printf("}");
                success = false;
            }
        }
    }

    utils::Mem::Destroy();
    utils::String::Allocator::Destroy();

    delete [] code;

    return success ? 1 : -1;
}

void handleAST(Context* ctx, ast_node* n) {
    Compiler c(ctx, n);
    CompilerOutput* out = c.compile();

    if (out) {
        printf("{\n");

        const auto& logs = c.getLogs();
        if (logs.size() > 0) {
            const char* logTps[] = { "info", "warning", "error" };
            printf("    \"logs\": [\n");
            for (u32 i = 0;i < logs.size();i++) {
                const auto& log = logs[i];
                const SourceLocation& src = log.node->tok->src;
                String ln = src.getSource()->getLine(src.getLine()).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");
                u32 wsc = 0;
                for (;wsc < ln.size();wsc++) {
                    if (isspace(ln[wsc])) continue;
                    break;
                }

                printf("        {\n");
                printf("            \"code\": \"C%d\",\n", log.code);
                printf("            \"type\": \"%s\",\n", logTps[log.type]);
                printf("            \"line\": %d,\n", src.getLine());
                printf("            \"col\": %d,\n", src.getCol());
                printf("            \"line_txt\": \"%s\",\n", ln.c_str() + wsc);

                printf("            \"line_idx\": \"");
                for (u32 i = 0;i < (src.getCol() - wsc);i++) printf(" ");
                printf("^\",\n");

                printf("            \"message\": \"%s\",\n", log.msg.c_str());
                printf("            \"ast\": ");
                log.node->json(3);
                printf("\n");
                printf("        }%s\n", (i == logs.size() - 1) ? "" : ",");
            }
            printf("    ],\n");
        } else printf("    \"logs\": [],\n");

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
                printf("            \"access\": \"%s\",\n", t->getAccessModifier() == public_access ? "public" : "private");
                printf("            \"fully_qualified_name\": \"%s\",\n", t->getFullyQualifiedName().c_str());
                printf("            \"destructor\": ");
                if (t->getDestructor()) {
                    printf("{\n");
                    printf("                \"name\": \"%s\",\n", t->getDestructor()->getFullyQualifiedName().c_str());
                    printf("                \"access\": \"%s\",\n", (t->getDestructor()->getAccessModifier() == private_access) ? "private" : "public");
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
                }

                const auto& bases = t->getBases();
                if (bases.size() > 0) {
                    printf("            \"inherits\": [\n");
                    for (u32 b = 0;b < bases.size();b++) {
                        const ffi::type_base& base = bases[b];
                        printf("                {\n");
                        printf("                    \"type\": \"%s\",\n", base.type->getFullyQualifiedName().c_str());
                        printf("                    \"access\": \"%s\",\n", (base.access == private_access) ? "private" : "public");
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
                        printf("                    \"access\": \"%s\",\n", (prop.access == private_access) ? "private" : "public");
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
                        printf("                    \"access\": \"%s\"\n", (method->getAccessModifier() == private_access) ? "private" : "public");
                        printf("                }%s\n", (m == methods.size() - 1) ? "" : ",");
                    }
                    printf("             ]\n");
                } else printf("            \"methods\": []");

                if (info.is_template) {
                    printf(",\n");
                    printf("            \"ast\": ");
                    ffi::TemplateType* tt = (ffi::TemplateType*)t;
                    tt->getAST()->json(3);
                    printf("\n");
                } else printf("\n");
                
                printf("        }%s\n", (i == types.size() - 1) ? "" : ",");
            }
            printf("    ],\n");
        } else printf("    \"types\": [],\n");

        const auto& funcs = c.getOutput()->getFuncs();
        if (funcs.size() > 0) {
            printf("    \"functions\": [\n");
            for (u32 i = 1;i < funcs.size();i++) {
                ffi::Function* f = funcs[i]->getOutput();
                if (!f) continue;
                
                printf("        {\n");
                printf("            \"id\": %d,\n", f->getId());
                printf("            \"name\": \"%s\",\n", f->getName().c_str());
                printf("            \"fully_qualified_name\": \"%s\",\n", f->getFullyQualifiedName().c_str());
                printf("            \"signature\": \"%s\",\n", f->getSignature()->getFullyQualifiedName().c_str());
                printf("            \"access\": \"%s\",\n", (f->getAccessModifier() == private_access) ? "private" : "public");
                printf("            \"is_method\": %s,\n", f->isMethod() ? "true" : "false");
                printf("            \"is_thiscall\": %s,\n", f->isThisCall() ? "true" : "false");
                
                ffi::FunctionType* sig = f->getSignature();
                const auto& args = sig->getArguments();
                if (args.size() > 0) {
                    printf("            \"args\": [\n");

                    static const char* argTpStr[] = {
                        "func_ptr",
                        "ret_ptr",
                        "ectx_ptr",
                        "this_ptr",
                        "value",
                        "ptr"
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
                    printf("            ]\n");
                } else printf("            \"code\": []\n");
                
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
                printf("            \"access\": \"%s\",\n", var.access == public_access ? "public" : "private");
                printf("            \"type\": \"%s\"\n", var.type->getFullyQualifiedName().c_str());
                printf("        }%s\n", (i == globals.size() - 1) ? "" : ",");
            }
            printf("    ],\n");
        } else printf("    \"globals\": [],\n");

        printf("    \"ast\": ");
        n->json(1);
        printf("\n}\n");
        delete out;
    }
}