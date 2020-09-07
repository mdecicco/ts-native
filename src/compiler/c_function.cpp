#include <compiler/context.h>
#include <parser/ast.h>
#include <vm_function.h>

namespace gjs {
	namespace compile {
		void function_declaration(context& ctx, parse::ast* n) {
		}

		void function_call(context& ctx, parse::ast* n) {
		}

		std::string arg_tp_str(const std::vector<vm_type*> types) {
			return "";
		}

		var call(context& ctx, vm_function* func, const std::vector<var>& args) {
			return ctx.dummy_var(func->signature.return_type);
		}

	};
};