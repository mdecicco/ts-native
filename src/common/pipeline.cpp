#include <common/pipeline.h>
#include <backends/backend.h>
#include <common/script_context.h>
#include <common/errors.h>
#include <common/script_module.h>
#include <common/script_function.h>
#include <backends/register_allocator.h>

#include <compiler/compile.h>
#include <lexer/lexer.h>
#include <parser/parse.h>
#include <parser/ast.h>

namespace gjs {
    compilation_output::compilation_output(u16 gpN, u16 fpN) : mod(nullptr) {
    }

    void compilation_output::insert(u64 addr, const compile::tac_instruction& i) {
        code.insert(code.begin() + addr, i);
        for (u32 i = 0;i < code.size();i++) {
            if (code[i].op == compile::operation::branch) {
                if (code[i].operands[1].imm_u() >= addr) code[i].operands[1].m_imm.u++;
            } else if (code[i].op == compile::operation::jump) {
                if (code[i].operands[0].imm_u() >= addr) code[i].operands[0].m_imm.u++;
            }
        }
    }

    void compilation_output::erase(u64 addr) {
        code.erase(code.begin() + addr);
        for (u32 i = 0;i < code.size();i++) {
            if (code[i].op == compile::operation::branch) {
                if (code[i].operands[1].imm_u() > addr) code[i].operands[1].m_imm.u--;
            } else if (code[i].op == compile::operation::jump) {
                if (code[i].operands[0].imm_u() > addr) code[i].operands[0].m_imm.u--;
            }
        }
    }


    pipeline::pipeline(script_context* ctx) : m_ctx(ctx) {
    }

    pipeline::~pipeline() {
    }

    script_module* pipeline::compile(const std::string& module, const std::string& code, backend* generator) {
        bool is_entry = m_importStack.size() == 0;
        if (is_entry) {
            m_log.errors.clear();
            m_log.warnings.clear();
            m_importStack.push_back({ source_ref("[internal]", "", 0, 0), module });
        }

        bool cyclic = false;
        for (u32 i = 0;i < m_importStack.size() - 1 && !cyclic;i++) cyclic = m_importStack[i].second == module;

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
            lex::tokenize(code, module, tokens);

            tree = parse::parse(m_ctx, module, tokens);
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
        out.mod = new script_module(m_ctx, module);

        try {
            compile::compile(m_ctx, tree, out);
            delete tree;
        } catch (error::exception& e) {
            if (is_entry) m_importStack.pop_back();
            delete tree;
            delete out.mod;
            throw e;
        } catch (std::exception& e) {
            if (is_entry) m_importStack.pop_back();
            delete tree;
            throw e;
        }

        if (m_log.errors.size() > 0) {
            for (u16 i = 0;i < out.funcs.size();i++) {
                delete out.funcs[i].func;
            }
            delete out.mod;
        }
        else {
            try {
                for (u8 i = 0;i < m_ir_steps.size();i++) {
                   m_ir_steps[i](m_ctx, out);
                }

                for (u16 i = 0;i < out.funcs.size();i++) {
                    if (!out.funcs[i].func) continue;
                    out.funcs[i].regs.m_gpc = generator->gp_count();
                    out.funcs[i].regs.m_fpc = generator->fp_count();
                    out.funcs[i].regs.process(i);
                }

                generator->generate(out);

                if (m_log.errors.size() == 0) {
                    out.mod->m_init = out.funcs[0].func;

                    for (u16 i = 1;i < out.funcs.size();i++) out.mod->add(out.funcs[i].func);
                    m_ctx->add(out.mod);
                } else {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        delete out.funcs[i].func;
                    }
                }
            } catch (error::exception& e) {
                for (u16 i = 0;i < out.funcs.size();i++) {
                    delete out.funcs[i].func;
                }
                out.funcs.clear();

                if (is_entry) m_importStack.pop_back();
                delete out.mod;
                throw e;
            } catch (std::exception& e) {
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