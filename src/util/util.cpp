#include <stdarg.h>
#include <gjs/util/util.h>
#include <gjs/common/script_context.h>
#include <gjs/backends/b_vm.h>
#include <gjs/common/script_function.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_module.h>
#include <gjs/common/errors.h>
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

    vector<string> get_lines(const string& str) {
        vector<string> out;

        string cur;
        for(size_t i = 0;i < str.length();i++) {
            if (str[i] == '\n') {
                out.push_back(cur);
                cur.clear();
                // handle \n\r line endings
                if (i + 1 < str.length() && str[i + 1] == '\r') i++;
                continue;
            } else if (str[i] == '\r') {
                out.push_back(cur);
                cur.clear();
                // handle \r\n line endings
                if (i + 1 < str.length() && str[i + 1] == '\n') i++;
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
        for (u32 i = 0;i < source.size();i++) {
            // match name
            if (name.find_first_of(' ') != std::string::npos) {
                // probably an operator
                std::vector<std::string> mparts = split(split(source[i]->name, ":")[1], " \t\n\r");
                std::vector<std::string> sparts = split(name, " \t\n\r");
                if (mparts.size() != sparts.size()) continue;
                bool matched = true;
                for (u32 i = 0;matched && i < mparts.size();i++) {
                    matched = mparts[i] == sparts[i];
                }
                if (!matched) continue;
            } else if (name != source[i]->name) continue;

            bool matches = ret ? source[i]->type->signature->return_type->id() == ret->id() : true;
            if (!matches) continue;

            if (source[i]->type->signature->explicit_argc != arg_types.size()) continue;

            matches = true;
            for (u8 a = 0;a < source[i]->type->signature->explicit_argc && matches;a++) {
                matches = (source[i]->type->signature->explicit_arg(a).tp->id() == arg_types[a]->id());
            }

            if (matches) return source[i];
        }

        return nullptr;
    }

    script_function* function_search(const std::string& name, const std::vector<script_function*>& source, const function_signature& sig) {
        for (u32 i = 0;i < source.size();i++) {
            // match name
            if (name.find_first_of(' ') != std::string::npos && source[i]->name.find_first_of(' ') != std::string::npos) {
                // probably an operator
                std::vector<std::string> mparts = split(split(source[i]->name, ":")[1], " \t\n\r");
                std::vector<std::string> sparts = split(name, " \t\n\r");
                if (mparts.size() != sparts.size()) continue;
                bool matched = true;
                for (u32 i = 0;matched && i < mparts.size();i++) {
                    matched = mparts[i] == sparts[i];
                }
                if (!matched) continue;
            } else if (name != source[i]->name) continue;

            bool matches = source[i]->type->signature->return_type->id() == sig.return_type->id();
            if (!matches) continue;

            if (source[i]->type->signature->explicit_argc != sig.explicit_argc) continue;

            matches = true;
            for (u8 a = 0;a < source[i]->type->signature->explicit_argc && matches;a++) {
                matches = (source[i]->type->signature->explicit_arg(a).tp->id() == sig.explicit_arg(a).tp->id());
            }

            if (matches) return source[i];
        }

        return nullptr;
    }

    void print_log(script_context* ctx, u32 context_line_count) {
        for (u8 i = 0;i < ctx->compiler()->log()->errors.size();i++) {
            compile_message& m = ctx->compiler()->log()->errors[i];
            log_file& f = ctx->compiler()->log()->files[m.src.module];

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

            for (i32 l = i32(m.src.line) - i32(context_line_count);l < m.src.line && l < f.lines.size();l++) {
                if (l < 0) continue;
                printf("%5d | %s\n", l + 1, f.lines[l].c_str());
            }

            printf("%5d | %s\n", m.src.line + 1, ln.c_str());
            for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
            printf("        ^\n");

            for (i32 l = i32(m.src.line) + 1;l < m.src.line + i32(context_line_count) + 1 && l < f.lines.size();l++) {
                printf("%5d | %s\n", l + 1, f.lines[l].c_str());
            }
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
            printf("%5d | %s\n", m.src.line + 1, ln.c_str());
            for (u32 i = 0;i < m.src.col - wscount;i++) printf(" ");
            printf("        ^\n");
        }
    }

    void print_code(vm_backend* ctx) {
        robin_hood::unordered_map<u64, script_function*> funcsByAddr;
        auto funcs = ctx->context()->functions();
        for (u32 i = 0;i < funcs.size();i++) {
            if (funcs[i]->is_host) continue;
            funcsByAddr[funcs[i]->access.entry] = funcs[i];
        }
        auto modules = ctx->context()->modules();
        for (u32 i = 0;i < modules.size();i++) {
            script_function* init = modules[i]->function("__init__");
            if (init) funcsByAddr[init->access.entry] = init;
        }

        char instr_fmt[32] = { 0 };
        u8 addr_w = snprintf(instr_fmt, 32, "%llx", ctx->code()->size());
        snprintf(instr_fmt, 32, " 0x%%%d.%dllX: %%-64s", addr_w, addr_w);
        
        std::string last_line = "";
        for (u64 i = 0;i < ctx->code()->size();i++) {
            script_function* f = nullptr;
            auto p = funcsByAddr.find(i);
            if (p != funcsByAddr.end()) f = p->getSecond();

            if (f) {
                u16 implicit_arg_count = 0;
                if (f->is_method_of) implicit_arg_count++;
                if (f->is_subtype_obj_ctor) implicit_arg_count++;
                if (f->type->signature->returns_on_stack) implicit_arg_count++;

                printf("\n[%s %s::%s(", f->type->signature->return_type->name.c_str(), f->owner->name().c_str(), f->name.c_str());
                for(u8 a = 0;a < f->type->signature->args.size();a++) {
                    if (a > 0) printf(", ");
                    auto& arg = f->type->signature->args[a];
                    if (arg.implicit != function_signature::argument::implicit_type::not_implicit) {
                        static const char* implicitArgNames[] = {
                            nullptr,
                            "@ctx",
                            "@this",
                            "@moduletype_id",
                            "@ret"
                        };
                        printf("%s %s -> $%s", arg.tp->name.c_str(), implicitArgNames[u8(arg.implicit)], register_str[u8(arg.loc)]);
                    } else {
                        printf("%s arg_%d -> $%s", arg.tp->name.c_str(), a, register_str[u8(arg.loc)]);
                    }
                }
                printf(")");

                if (f->type->signature->return_type->name == "void") printf(" -> null");
                else if (f->type->signature->returns_on_stack) printf(" -> @ret");
                else printf(" -> $%s", register_str[u8(f->type->signature->return_loc)]);
                printf("]\n");
            }

            instruction ins = (*ctx->code())[i];
            printf(instr_fmt, i, ins.to_string(ctx).c_str());
            auto src = ctx->map()->get(i);
            if (src.line_text != last_line) {
                printf("; %s", src.line_text.c_str());
                last_line = src.line_text;
            } else if (ins.instr() == vm_instruction::jal) {
                script_function* f = ctx->context()->function((u32)ins.imm_u());
                if (!f) printf("; <- Bad function ID");
            }
            printf("\n");
        }
    }

    void debug_ir_step(script_context* ctx, compilation_output& in, u16 f) {
        script_function* fn = in.funcs[f].func;
        if (!fn) return;
        printf("\n[%s %s(", fn->type->signature->return_type->name.c_str(), fn->name.c_str());
        for(u8 a = 0;a < fn->type->signature->args.size();a++) {
            if (a > 0) printf(", ");
            printf("%s arg_%d", fn->type->signature->args[a].tp->name.c_str(), a);
        }
        printf(")]\n");

        for (u32 i = 0;i < in.funcs[f].code.size();i++) {
            printf("%3.3d: %s\n", i, in.funcs[f].code[i].to_string().c_str());
        }
    }

    script_type* resolve_moduletype(u64 moduletype) {
        return script_context::current()->module(extract_left_u32(moduletype))->types()->get(extract_right_u32(moduletype));
    }
};