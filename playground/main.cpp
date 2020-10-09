#include <gjs.h>
#include <stdio.h>

using namespace gjs;

void print_f32(u16 i, f32 f) {
    printf("%d: %f\n", i, f);
}

// https://www.notion.so/3ccc9d8dba114bf8acfbbe238cb729e3?v=1a0dff3b20ba4f678bc7f5866665c4df
int main(int arg_count, const char** args) {
    script_context ctx;

    //ctx.compiler()->add_ir_step(debug_ir_step);

    ctx.bind(print_f32, "print_f32");

    printf("------------IR code-------------\n");
    script_module* mod = ctx.add_code(
        "C:/Users/miguel/Documents/programming/gjs/playground/bin/itabc.gjs",
        "f32 abc = 4.5f;\n"
        "void it() { print_f32(0, abc); }\n"
        "abc = 1.0f;\n"
    );

    script_module* mod1 = ctx.add_code(
        "C:/Users/miguel/Documents/programming/gjs/playground/bin/test1.gjs",
        "import 'itabc' as ayy;\n"
        "import { it as stuff } from 'test';\n"
        // import { it, abc } from 'test';
        // import { it as thing, abc as what } from 'test';
        "void it1() {\n"
            "ayy.it();\n"
            "stuff();\n"
            "print_f32(0, ayy.abc);\n"
        "}\n"
    );

    if (mod && mod1) {
        printf("------------VM code-------------\n");
        print_code((vm_backend*)ctx.generator());

        printf("-------------result-------------\n");
        mod->init();
        mod1->init();

        //((vm_backend*)ctx.generator())->log_instructions(true);
        ctx.call<void>(mod->function("it"), nullptr);
        mod->set_local("abc", 3.14f);
        ctx.call<void>(mod1->function("it1"), nullptr);
    } else {
        print_log(&ctx);
    }
    return 0;
}