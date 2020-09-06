#pragma once

namespace gjs {
	struct compile_context;
	struct var;

	namespace parse {
		struct ast;
	};

	var* compile_expression(compile_context& ctx, parse::ast* node, var* dest);
};