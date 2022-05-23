#include <gs/utils/ProgramSource.h>
#include <gs/compiler/lexer.h>
#include <gs/compiler/parser.h>

#include <utils/Array.hpp>

using namespace gs;
using namespace compiler;
using namespace utils;


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
        ProgramSource src = ProgramSource(argv[1], code);
        Lexer l(&src);
        ParserState ps(&l);
        ast_node* n = ps.parse();

        const auto& errors = ps.errors();
        if (errors.size() > 0) {
            success = false;

            printf("The following errors occurred while parsing the input:\n");
            for (const auto& e : errors) {
                const SourceLocation& src = e.src.src;
                String ln = src.getSource()->getLine(src.getLine()).clone();
                ln.replaceAll("\n", "");
                ln.replaceAll("\r", "");
                printf("Line %d, col %d: %s\n", src.getLine(), src.getCol(), e.text.c_str());
                u32 wsc = 0;
                for (u32 i = 0;i < ln.size();i++) {
                    if (isspace(ln[i])) continue;
                    break;
                }
                printf("> %s\n  ", ln.c_str() + wsc);
                for (u32 i = 0;i < (src.getCol() - wsc);i++) {
                    printf(" ");
                }
                printf("^\n\n");
            }
        } else {
            if (n) n->json();
            else {
                printf("An unknown error occurred...\n");
                success = false;
            }
        }
    }

    utils::Mem::Destroy();
    utils::String::Allocator::Destroy();

    delete [] code;

    return success ? 1 : -1;
}