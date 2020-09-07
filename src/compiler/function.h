#pragma once
#include <compiler/variable.h>

namespace gjs {
	class vm_function;

	namespace compile {
		struct context;

		std::string arg_tp_str(const std::vector<vm_type*> types);

		var call(context& ctx, vm_function* func, const std::vector<var>& args);
	};
};