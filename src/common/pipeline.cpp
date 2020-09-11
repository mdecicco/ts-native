#include <common/pipeline.h>
#include <backends/backend.h>
#include <common/context.h>
#include <common/errors.h>
#include <backends/register_allocator.h>

#include <compiler/compile.h>
#include <lexer/lexer.h>
#include <parser/parse.h>

namespace gjs {
    pipeline::pipeline(script_context* ctx) : m_ctx(ctx) {
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

        compilation_output out;
        try {
            compile::compile(m_ctx, tree, out);
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
                    m_ir_steps[i](m_ctx, out);
                }

                register_allocator ro(out, generator->gp_count(), generator->fp_count());
                ro.process();

            } catch (error::exception& e) {
                throw e;
            } catch (std::exception& e) {
                throw e;
            }

            generator->generate(out);
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