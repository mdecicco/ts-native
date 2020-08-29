#pragma once
#include <parse.h>

namespace gjs {
	struct compile_context;
	struct var;

	var* compile_expression(compile_context& ctx, ast_node* node, var* dest);
};