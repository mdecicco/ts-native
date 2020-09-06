#include <parser/parse.h>
#include <parser/context.h>
#include <parser/ast.h>
#include <errors.h>

namespace gjs {
	using tt = lex::token_type;
	using exc = error::exception;
	using ec = error::ecode;
	using token = lex::token;

	namespace parse {
		using nt = ast::node_type;

		ast* class_declaration(context& ctx) {
			return nullptr;
		}

		ast* format_declaration(context& ctx) {
			return nullptr;
		};
	};
};