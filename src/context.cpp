#include <context.h>

namespace gjs {
	vm_context::vm_context() {
	}
	vm_context::~vm_context() {
	}

	void vm_context::add(const std::string& name, wrapped_function* func) {
	}
	void vm_context::add(const std::string& name, wrapped_class* cls) {
	}

	wrapped_function* vm_context::function(const std::string& name) {
		return nullptr;
	}
	wrapped_class* vm_context::prototype(const std::string& name) {
		return nullptr;
	}
};
