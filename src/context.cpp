#include <context.h>
#include <bind.h>
#include <parse.h>
#include <compile.h>
#include <asmjit/asmjit.h>

namespace gjs {
	runtime_exception::runtime_exception(vm_context* ctx, const std::string& _text) : text(_text), raised_from_script(true), line(0), col(0) {
		if (ctx->is_executing()) {
			source_map::src_info info = ctx->map()->get((address)ctx->state()->registers[(integer)vm_register::ip]);
			file = info.file;
			lineText = info.lineText;
			line = info.line;
			col = info.col;
		}
		else raised_from_script = false;
	}

	runtime_exception::runtime_exception(const std::string& _text) : text(_text), raised_from_script(false), line(0), col(0) {
	}

	runtime_exception::~runtime_exception() {
	}



	vm_context::vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size) : 
		m_vm(this, alloc, stack_size, mem_size), m_instructions(alloc), m_is_executing(false),
		m_catch_exceptions(false),m_types(this)
	{
		m_jit = new asmjit::JitRuntime();
		m_instructions += encode(vm_instruction::term);

		vm_type* tp = nullptr;
		tp = m_types.add("integer", typeid(integer).name());
		tp->is_primitive = true;
		tp->size = sizeof(integer);

		tp = m_types.add("decimal", typeid(decimal).name());
		tp->is_primitive = true;
		tp->size = sizeof(decimal);

		tp = m_types.add("string", "char");
		tp->is_primitive = false;
		tp->size = sizeof(char*);

		tp = m_types.add("void", "void");
		tp->is_primitive = true;
		tp->size = 0;
	}

	vm_context::~vm_context() {
		delete m_jit;
	}

	void vm_context::add(const std::string& name, bind::wrapped_function* func) {
	}

	void vm_context::add(const std::string& name, bind::wrapped_class* cls) {
	}

	void vm_context::add(const std::string& name, bind::script_function* func) {
		m_script_functions[name] = func;
	}

	void vm_context::add(vm_function* func) {
		u64 addr = 0;
		if (func->is_host) addr = func->access.wrapped->address;
		else addr = func->access.entry;

		if (m_funcs_by_addr.count(addr) > 0) {
			throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
		}

		m_funcs_by_addr[addr] = func;
		m_funcs[func->name] = func;
	}

	vm_function* vm_context::function(const std::string& name) {
		auto it = m_funcs.find(name);
		if (it == m_funcs.end()) return nullptr;
		return it->getSecond();
	}
	vm_function* vm_context::function(u64 address) {
		auto it = m_funcs_by_addr.find(address);
		if (it == m_funcs_by_addr.end()) return nullptr;
		return it->getSecond();
	}
	std::vector<vm_function*> vm_context::all_functions() {
		std::vector<vm_function*> out;
		for (auto i = m_funcs.begin();i != m_funcs.end();++i) {
			out.push_back(i->getSecond());
		}
		return out;
	}

	bind::wrapped_function* vm_context::host_function(const std::string& name) {
		return nullptr;
	}

	bind::wrapped_class* vm_context::host_prototype(const std::string& name) {
		return nullptr;
	}

	bind::script_function* vm_context::_function(const std::string& name) {
		return m_script_functions[name];
	}

	std::vector<bind::wrapped_class*> vm_context::host_prototypes() {
		std::vector<bind::wrapped_class*> out;
		for (auto i = m_host_classes.begin();i != m_host_classes.end();++i) {
			out.push_back(i->getSecond());
		}
		return out;
	}

	std::vector<bind::wrapped_function*> vm_context::host_functions() {
		std::vector<bind::wrapped_function*> out;
		for (auto i = m_host_functions.begin();i != m_host_functions.end();++i) {
			out.push_back(i->getSecond());
		}
		return out;
	}

	void vm_context::add_code(const std::string& filename, const std::string& code) {
		try {
			ast_node* ast = parse_source(this, filename, code);
			compile_ast(this, ast, &m_instructions);
		} catch (parse_exception& e) {
			if (!m_catch_exceptions) throw e;
			printf("%s:%d:%d: %s\n", e.file.c_str(), e.line, e.col, e.text.c_str());
			std::string ln = "";
			u32 wscount = 0;
			bool reachedText = false;
			for (u32 i = 0;i < e.lineText.length();i++) {
				if (isspace(e.lineText[i]) && !reachedText) wscount++;
				else {
					reachedText = true;
					ln += e.lineText[i];
				}
			}
			printf("%s\n", ln.c_str());
			for (u32 i = 0;i < e.col - wscount;i++) printf(" ");
			printf("^\n");
		} catch (compile_exception& e) {
			if (!m_catch_exceptions) throw e;
			printf("%s:%d:%d: %s\n", e.file.c_str(), e.line, e.col, e.text.c_str());
			std::string ln = "";
			u32 wscount = 0;
			bool reachedText = false;
			for (u32 i = 0;i < e.lineText.length();i++) {
				if (isspace(e.lineText[i]) && !reachedText) wscount++;
				else {
					reachedText = true;
					ln += e.lineText[i];
				}
			}
			printf("%s\n", ln.c_str());
			for (u32 i = 0;i < e.col - wscount;i++) printf(" ");
			printf("^\n");
		} catch (std::exception& e) {
			if (!m_catch_exceptions) throw e;
			printf("Caught exception: %s\n", e.what());
		}
	}

	void vm_context::execute(address entry) {
		m_is_executing = true;

		try {
			m_vm.execute(m_instructions, entry);
		} catch (runtime_exception& e) {
			if (!m_catch_exceptions) throw e;
			if (e.raised_from_script) {
				printf("%s:%d:%d: %s\n", e.file.c_str(), e.line, e.col, e.text.c_str());
				std::string ln = "";
				u32 wscount = 0;
				bool reachedText = false;
				for (u32 i = 0;i < e.lineText.length();i++) {
					if (isspace(e.lineText[i]) && !reachedText) wscount++;
					else {
						reachedText = true;
						ln += e.lineText[i];
					}
				}
				printf("%s\n", ln.c_str());
				for (u32 i = 0;i < e.col - wscount;i++) printf(" ");
				printf("^\n");
			} else printf("%s\n", e.text.c_str());
		} catch (std::exception& e) {
			if (!m_catch_exceptions) throw e;
			printf("Caught exception: %s\n", e.what());
		}

		m_is_executing = false;
	}
};
