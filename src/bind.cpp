#include <bind.h>

namespace gjs {
	namespace bind {
		std::any& get_input_param(std::vector<std::any>& params, u32 idx) {
			return params[idx];
		}

	};
};