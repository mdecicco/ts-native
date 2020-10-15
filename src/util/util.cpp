#include <stdarg.h>
#include <util/util.h>
#include <common/script_context.h>
#include <backends/vm.h>
#include <common/script_function.h>
#include <common/script_type.h>
#include <common/script_module.h>
#include <common/errors.h>
#include <stdio.h>
using namespace std;

namespace gjs {
    vector<string> split(const string& str, const string& delimiters) {
        vector<string> out;

        string cur;
        bool lastWasDelim = false;
        for(size_t i = 0;i < str.length();i++) {
            bool isDelim = false;
            for(unsigned char d = 0;d < delimiters.length() && !isDelim;d++) {
                isDelim = str[i] == delimiters[d];
            }

            if (isDelim) {
                if (!lastWasDelim) {
                    out.push_back(cur);
                    cur = "";
                    lastWasDelim = true;
                }
                continue;
            }

            cur += str[i];
            lastWasDelim = false;
        }

        if (cur.length() > 0) out.push_back(cur);
        return out;
    }

    vector<string> get_lines(const string& str, const string& delimiters) {
        vector<string> out;

        string cur;
        bool lastWasDelim = false;
        for(size_t i = 0;i < str.length();i++) {
            bool isDelim = false;
            for(unsigned char d = 0;d < delimiters.length() && !isDelim;d++) {
                isDelim = str[i] == delimiters[d];
            }

            if (isDelim) {
                out.push_back(cur);
                cur = "";
                continue;
            }

            cur += str[i];
        }

        if (cur.length() > 0) out.push_back(cur);
        return out;
    }

    string format(const char* fmt, ...) {
        va_list l;
        va_start(l, fmt);
        char out[1024] = { 0 };
        vsnprintf(out, 1024, fmt, l);
        va_end(l);
        return out;
    }

    u32 hash(const std::string& str) {
        u32 h = 0;
        for (u32 i = 0;i < str.length();i++) h = 37 * h + str[i];
        return h + (h >> 5);
    }

    script_function* function_search(const std::string& name, const std::vector<script_function*>& source, script_type* ret, const std::vector<script_type*>& arg_types) {
        for (u8 i = 0;i < source.size();i++) {
            bool matches = ret ? source[i]->signature.return_type->id() == ret->id() : true;
            if (!matches) continue;

            if (source[i]->signature.arg_types.size() != arg_types.size()) continue;

            matches = true;
            for (u8 a = 0;a < source[i]->signature.arg_types.size() && matches;a++) {
                matches = (source[i]->signature.arg_types[a]->id() == arg_types[a]->id());
            }

            if (matches) return source[i];
        }

        return nullptr;
    }

    void print_log(script_context* ctx) {
        for (u8 i = 0;i < ctx->compiler()->log()->errors.size();i++) {
            compile_message& m = ctx->compiler()->log()->errors[i];

            // useless
            if (m.code.e == error::ecode::c_compile_finished_with_errors) continue;

            printf("Error: %s\nIn module: %s\n", m.text.c_str(), m.src.module.c_str());
            std::string ln = "";
            u32 wscount = 0;
            bool reachedText = false;
            for (u32 i = 0;i < m.src.line_text.length();i++) {
                if (isspace(m.src.line_text[i]) && !reachedText) wscount++;
                else {
                    reachedText = true;
                    ln += m.src.line_text[i];
                }
            }
            if (wscount > m.src.col) wscount = m.src.col;
            printf("%5d | %s\n", m.src.line, ln.c_str());
            for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
            printf("        ^\n");
        }

        for (u8 i = 0;i < ctx->compiler()->log()->warnings.size();i++) {
            compile_message& m = ctx->compiler()->log()->warnings[i];
            printf("Warning: %s\nIn module: %s\n", m.text.c_str(), m.src.module.c_str());
            std::string ln = "";
            u32 wscount = 0;
            bool reachedText = false;
            for (u32 i = 0;i < m.src.line_text.length();i++) {
                if (isspace(m.src.line_text[i]) && !reachedText) wscount++;
                else {
                    reachedText = true;
                    ln += m.src.line_text[i];
                }
            }
            if (wscount > m.src.col) wscount = m.src.col;
            printf("%5d | %s\n", m.src.line, ln.c_str());
            for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
            printf("        ^\n");
        }
    }

    void print_code(vm_backend* ctx) {
        robin_hood::unordered_map<u64, script_function*> module_initializers;
        auto modules = ctx->context()->modules();
        for (u32 i = 0;i < modules.size();i++) {
            script_function* init = modules[i]->function("__init__");
            if (init) module_initializers[init->access.entry] = init;
        }

        char instr_fmt[32] = { 0 };
        u8 addr_w = snprintf(instr_fmt, 32, "%llx", ctx->code()->size());
        snprintf(instr_fmt, 32, " 0x\%%%d.%dllX: \%%-32s", addr_w, addr_w);
        
        std::string last_line = "";
        for (u64 i = 0;i < ctx->code()->size();i++) {
            script_function* f = ctx->context()->function(i);
            if (!f) {
                auto init = module_initializers.find(i);
                if (init != module_initializers.end()) f = init->getSecond();
            }

            if (f) {
                printf("\n[%s %s::%s(", f->signature.return_type->name.c_str(), f->owner->name().c_str(), f->name.c_str());
                for(u8 a = 0;a < f->signature.arg_types.size();a++) {
                    if (a > 0) printf(", ");
                    printf("%s arg_%d -> $%s", f->signature.arg_types[a]->name.c_str(), a, register_str[u8(f->signature.arg_locs[a])]);
                }
                printf(")");

                if (f->signature.return_type->name == "void") printf(" -> null");
                else {
                    if (f->signature.returns_on_stack) {
                        printf(" -> $%s (stack)", register_str[u8(f->signature.return_loc)]);
                    } else {
                        printf(" -> $%s", register_str[u8(f->signature.return_loc)]);
                    }
                }
                printf("]\n");
            }

            instruction ins = (*ctx->code())[i];
            printf(instr_fmt, i, ins.to_string(ctx).c_str());
            auto src = ctx->map()->get(i);
            if (src.line_text != last_line) {
                printf("; %s", src.line_text.c_str());
                last_line = src.line_text;
            } else if (ins.instr() == vm_instruction::jal) {
                std::string str;
                script_function* f = ctx->context()->function(ins.imm_u());
                if (f) {
                    str += f->signature.return_type->name + " " + f->owner->name() + "::" + f->name + "(";
                    for (u8 a = 0;a < f->signature.arg_types.size();a++) {
                        if ((a > 0 && !f->is_method_of) || (f->is_method_of && a > 1)) str += ", ";
                        if (a == 0 && f->is_method_of) continue; // skip implicit 'this' parameter
                        str += f->signature.arg_types[a]->name;
                    }
                    str += ")";
                } else str = "Bad function address";
                printf("; <- %s", str.c_str());
            }
            printf("\n");
        }
    }

    void debug_ir_step(script_context* ctx, compilation_output& in) {
        for (u32 i = 0;i < in.code.size();i++) {
            printf("%3.3d: %s\n", i, in.code[i].to_string().c_str());
        }
    }
};