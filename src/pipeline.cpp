#include <pipeline.h>
#include <vm/context.h>
#include <errors.h>

#include <compiler/compile.h>
#include <lexer/lexer.h>
#include <parser/parse.h>

namespace gjs {
    pipeline::pipeline(vm_context* ctx) : m_ctx(ctx) {
    }

    pipeline::~pipeline() {
    }

    bool pipeline::compile(const std::string& file, const std::string& code, backend* generator) {
        m_log.errors.clear();
        m_log.warnings.clear();

        std::vector<lex::token> tokens;
        lex::tokenize(code, file, tokens);

        parse::ast* tree = parse::parse(m_ctx, file, tokens);

        try {
            for (u8 i = 0;i < m_ast_steps.size();i++) {
                m_ast_steps[i](m_ctx, tree);
            }
        } catch (error::exception& e) {
            delete tree;
            throw e;
        } catch (std::exception& e) {
            delete tree;
            throw e;
        }

        ir_code ir;
        try {
            compile::compile(m_ctx, tree, ir);
            delete tree;
        } catch (error::exception& e) {
            delete tree;
            throw e;
        } catch (std::exception& e) {
            delete tree;
            throw e;
        }

        if (m_log.errors.size() > 0) {
        }
        else {
            try {
                for (u8 i = 0;i < m_ir_steps.size();i++) {
                    m_ir_steps[i](m_ctx, ir);
                }
            } catch (error::exception& e) {
                throw e;
            } catch (std::exception& e) {
                throw e;
            }

            generator->generate(ir);
        }


        return m_log.errors.size() == 0;
    }

    void pipeline::add_ir_step(ir_step_func step) {
        m_ir_steps.push_back(step);
    }

    void pipeline::add_ast_step(ast_step_func step) {
        m_ast_steps.push_back(step);
    }
};