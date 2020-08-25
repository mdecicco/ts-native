#include <pipeline.h>
#include <parse.h>
#include <compile.h>
#include <context.h>

namespace gjs {
	pipeline::pipeline(vm_context* ctx) : m_ctx(ctx) {
	}

	pipeline::~pipeline() {
	}

	void pipeline::compile(const std::string& file, const std::string& code) {
		ast_node* ast = parse_source(m_ctx, file, code);
		for (u8 i = 0;i < m_ast_steps.size();i++) {
			m_ast_steps[i](m_ctx, ast);
		}

		u32 new_code_starts_at = m_ctx->code()->size();
		compile_ast(m_ctx, ast, m_ctx->code(), m_ctx->map());
		for (u8 i = 0;i < m_ir_steps.size();i++) {
			m_ir_steps[i](m_ctx, *m_ctx->code(), m_ctx->map(), new_code_starts_at);
		}

		delete ast;
	}

	void pipeline::add_ir_step(ir_step_func step) {
		m_ir_steps.push_back(step);
	}

	void pipeline::add_ast_step(ast_step_func step) {
		m_ast_steps.push_back(step);
	}
};