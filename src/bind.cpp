#include <bind.h>

namespace gjs {
	namespace bind {
		void set_arguments(vm_context* ctx, vm_function* func, u8 idx) { }

		wrapped_class::~wrapped_class() {
			for (auto i = properties.begin();i != properties.end();++i) {
				delete i->getSecond();
			}
			properties.clear();
		}

		declare_input_binding(integer, context, in, reg_ptr) {
			(*(integer*)reg_ptr) = in;
		}

		declare_output_binding(integer, context, out, reg_ptr) {
			*out = *(integer*)reg_ptr;
		}

		declare_input_binding(decimal, context, in, reg_ptr) {
			(*(decimal*)reg_ptr) = in;
		}

		declare_output_binding(decimal, context, out, reg_ptr) {
			*out = *(decimal*)reg_ptr;
		}
	};
};