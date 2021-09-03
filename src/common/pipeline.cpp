#include <gjs/common/pipeline.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_context.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_module.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_enum.h>
#include <gjs/backends/register_allocator.h>

#include <gjs/compiler/compile.h>
#include <gjs/lexer/lexer.h>
#include <gjs/parser/parse.h>
#include <gjs/parser/ast.h>

namespace gjs {
    compilation_output::compilation_output(u16 gpN, u16 fpN) : mod(nullptr) {
    }

    void compilation_output::func_def::insert(u64 addr, const compile::tac_instruction& i) {
        compilation_output::insert(code, addr, i);
    }

    void compilation_output::func_def::erase(u64 addr) {
        compilation_output::erase(code, addr);
    }

    void compilation_output::insert(compilation_output::ir_code& code, u64 addr, const compile::tac_instruction& i) {
        code.insert(code.begin() + addr, i);
    }

    void compilation_output::erase(compilation_output::ir_code& code, u64 addr) {
        code.erase(code.begin() + addr);
    }


    pipeline::pipeline(script_context* ctx) : m_ctx(ctx) {
    }

    pipeline::~pipeline() {
    }

    script_module* pipeline::compile(const std::string& module_name, const std::string& module_path, const std::string& code, backend* generator) {
        bool is_entry = m_importStack.size() == 0;
        if (is_entry) {
            m_log.files.clear();
            m_log.errors.clear();
            m_log.warnings.clear();
            m_importStack.push_back({ source_ref("[internal]", "", 0, 0), module_path });
        }

        bool cyclic = false;
        for (u32 i = 0;i < m_importStack.size() - 1 && !cyclic;i++) cyclic = m_importStack[i].second == module_path;

        if (cyclic) {
            std::string path;
            for (u32 i = 1;i < m_importStack.size();i++) {
                auto& src = m_importStack[i].first;
                path += format("> %s\n> %5d | %s\n", src.module.c_str(), src.line, src.line_text.c_str());
            }

            throw error::exception(error::ecode::p_cyclic_imports, m_importStack.back().first, path.c_str());
        }

        parse::ast* tree = nullptr;

        try {
            std::vector<lex::token> tokens;
            lex::tokenize(code, module_path, tokens, m_log.files[module_path] = log_file());

            tree = parse::parse(m_ctx, module_path, tokens);
            for (u8 i = 0;i < m_ast_steps.size();i++) {
                m_ast_steps[i](m_ctx, tree);
            }
        } catch (error::exception& e) {
            if (is_entry) m_importStack.pop_back();
            if (tree) delete tree;
            throw e;
        } catch (std::exception& e) {
            if (is_entry) m_importStack.pop_back();
            if (tree) delete tree;
            throw e;
        }

        compilation_output out(generator->gp_count(), generator->fp_count());
        out.mod = new script_module(m_ctx, module_name, module_path);

        try {
            compile::compile(m_ctx, tree, out);
            delete tree;
        } catch (error::exception& e) {
            if (is_entry) m_importStack.pop_back();
            delete tree;
            for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
            for (u16 i = 0;i < out.funcs.size();i++) {
                delete out.funcs[i].func;
            }
            delete out.mod;
            throw e;
        } catch (std::exception& e) {
            if (is_entry) m_importStack.pop_back();
            delete tree;
            for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
            for (u16 i = 0;i < out.funcs.size();i++) {
                delete out.funcs[i].func;
            }
            throw e;
        }

        if (m_log.errors.size() > 0) {
            for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
            for (u16 i = 0;i < out.funcs.size();i++) {
                delete out.funcs[i].func;
            }
            delete out.mod;
        }
        else {
            try {
                for (u16 f = 0;f < out.funcs.size();f++) {
                    for (u8 i = 0;i < m_ir_steps.size();i++) {
                       m_ir_steps[i](m_ctx, out, f);
                    }
                }

                if (generator->perform_register_allocation()) {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        if (!out.funcs[i].func) continue;
                        out.funcs[i].regs.m_gpc = generator->gp_count();
                        out.funcs[i].regs.m_fpc = generator->fp_count();
                        out.funcs[i].regs.process(i);
                    }
                }

                generator->generate(out);

                if (m_log.errors.size() == 0) {
                    out.mod->m_init = out.funcs[0].func;

                    for (u16 i = 1;i < out.funcs.size();i++) out.mod->add(out.funcs[i].func);
                    for (u16 i = 0;i < out.enums.size();i++) out.mod->m_enums.push_back(out.enums[i]);
                    m_ctx->add(out.mod);
                } else {
                    for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        delete out.funcs[i].func;
                    }
                }
            } catch (error::exception& e) {
                for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
                for (u16 i = 0;i < out.funcs.size();i++) {
                    delete out.funcs[i].func;
                }
                out.funcs.clear();

                if (is_entry) m_importStack.pop_back();
                delete out.mod;
                throw e;
            } catch (std::exception& e) {
                for (u16 i = 0;i < out.enums.size();i++) delete out.enums[i];
                for (u16 i = 0;i < out.funcs.size();i++) {
                    delete out.funcs[i].func;
                }
                out.funcs.clear();

                if (is_entry) m_importStack.pop_back();
                delete out.mod;
                throw e;
            }
        }

        if (is_entry) m_importStack.pop_back();
        return m_log.errors.size() == 0 ? out.mod : nullptr;
    }

    void pipeline::push_import(const source_ref& ref, const std::string& imported) {
        m_importStack.push_back({ ref, imported });
    }

    void pipeline::pop_import() {
        m_importStack.pop_back();
    }

    void pipeline::add_ir_step(ir_step_func step) {
        m_ir_steps.push_back(step);
    }

    void pipeline::add_ast_step(ast_step_func step) {
        m_ast_steps.push_back(step);
    }
};