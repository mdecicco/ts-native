#include <context.h>
#include <bind.h>

namespace gjs {
	vm_context::vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size) : m_vm(alloc, stack_size, mem_size), m_instructions(alloc) {
	}
	vm_context::~vm_context() {
	}

	void vm_context::add(const std::string& name, bind::wrapped_function* func) {
	}
	void vm_context::add(const std::string& name, bind::wrapped_class* cls) {
	}
	void vm_context::add(const std::string& name, script_function* func) {
		m_script_functions[name] = func;
	}

	bind::wrapped_function* vm_context::host_function(const std::string& name) {
		return nullptr;
	}
	bind::wrapped_class* vm_context::host_prototype(const std::string& name) {
		return nullptr;
	}
	script_function* vm_context::function(const std::string& name) {
		return m_script_functions[name];
	}

	void vm_context::execute(address entry) {
		m_vm.execute(m_instructions, entry);
	}

	void set_arguments(script_function* func, u8 idx) {
	}

	template<>
	void from_reg<integer>(integer* out, u64* reg_ptr) {
		*out = *(integer*)reg_ptr;
	}

	template<>
	void to_reg<integer>(const integer& in, u64* reg_ptr) {
		(*(integer*)reg_ptr) = in;
	}
};
