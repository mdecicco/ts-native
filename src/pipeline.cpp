#include <pipeline.h>
#include <parse.h>
#include <compiler/compiler.h>
#include <context.h>
#include <lexer/lexer.h>
#include <parser/parse.h>

namespace gjs {
	pipeline::pipeline(vm_context* ctx) : m_ctx(ctx) {
	}

	pipeline::~pipeline() {
	}

	bool pipeline::compile(const std::string& file, const std::string& code) {
		m_log.errors.clear();
		m_log.warnings.clear();

		/*
		std::vector<lex::token> tokens;
		lex::tokenize(code, file, tokens);

		ast_node* ast = parse::parse(m_ctx, file, tokens);
		*/

		ast_node* ast = parse_source(m_ctx, file, code);

		try {
			for (u8 i = 0;i < m_ast_steps.size();i++) {
				m_ast_steps[i](m_ctx, ast);
			}
		} catch (compile_exception& e) {
			delete ast;
			throw e;
		} catch (std::exception& e) {
			delete ast;
			throw e;
		}

		u32 new_code_starts_at = m_ctx->code()->size();

		try {
			compile_ast(m_ctx, ast, m_ctx->code(), m_ctx->map(), &m_log);
			delete ast;
		} catch (compile_exception& e) {
			m_ctx->code()->remove(new_code_starts_at, m_ctx->code()->size());
			delete ast;
			throw e;
		} catch (std::exception& e) {
			m_ctx->code()->remove(new_code_starts_at, m_ctx->code()->size());
			delete ast;
			throw e;
		}

		if (m_log.errors.size() > 0) m_ctx->code()->remove(new_code_starts_at, m_ctx->code()->size());
		else {
			try {
				for (u8 i = 0;i < m_ir_steps.size();i++) {
					m_ir_steps[i](m_ctx, *m_ctx->code(), m_ctx->map(), new_code_starts_at);
				}
			} catch (compile_exception& e) {
				m_ctx->code()->remove(new_code_starts_at, m_ctx->code()->size());
				throw e;
			} catch (std::exception& e) {
				m_ctx->code()->remove(new_code_starts_at, m_ctx->code()->size());
				throw e;
			}
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