#include <bind.h>
#include <context.h>

namespace gjs {
	type_manager::type_manager(vm_context* ctx) : m_ctx(ctx) {
	}

	type_manager::~type_manager() {
	}

	vm_type* type_manager::get(const std::string& name) {
		if (m_types.count(name) == 0) return nullptr;
		return m_types[name];
	}

	vm_type* type_manager::add(const std::string& name, const std::string& internal_name) {
		if (m_types.count(internal_name) > 0) {
			throw bind_exception(format("Type '%s' already bound", name.c_str()));
		}
		vm_type* t = new vm_type();
		t->name = name;
		t->internal_name = internal_name;

		m_types[internal_name] = t;

		return t;
	}

	void type_manager::finalize_class(bind::wrapped_class* wrapped) {
		auto it = m_types.find(wrapped->internal_name);
		if (it == m_types.end()) {
			throw bind_exception(format("Type '%s' not found and can not be finalized", wrapped->name.c_str()));
		}

		vm_type* t = it->getSecond();
		t->m_wrapped = wrapped;

		t->size = wrapped->size;
		t->constructor = wrapped->ctor ? new vm_function(this, wrapped->ctor) : nullptr;
		t->destructor = wrapped->dtor ? new vm_function(this, wrapped->dtor) : nullptr;

		for (auto i = wrapped->properties.begin();i != wrapped->properties.end();++i) {
			t->properties.push_back({
				i->getSecond()->flags,
				i->getFirst(),
				get(i->getSecond()->type.name()),
				i->getSecond()->offset,
				i->getSecond()->getter ? new vm_function(this, i->getSecond()->getter) : nullptr,
				i->getSecond()->setter ? new vm_function(this, i->getSecond()->setter) : nullptr
			});
		}

		for (auto i = wrapped->methods.begin();i != wrapped->methods.end();++i) {
			t->methods.push_back(new vm_function(this, i->getSecond()));
		}
	}

	std::vector<vm_type*> type_manager::all() {
		std::vector<vm_type*> out;
		for (auto i = m_types.begin();i != m_types.end();++i) {
			out.push_back(i->getSecond());
		}
		return out;
	}

	vm_function::vm_function(vm_context* ctx, const std::string _name, address addr) {
		m_ctx = ctx;
		name = _name;
		access.entry = addr;
		is_host = false;
	}

	vm_function::vm_function(type_manager* mgr, bind::wrapped_function* wrapped) {
		m_ctx = mgr->m_ctx;
		signature.return_loc = vm_register::v0;
		name = wrapped->name;
		signature.text = wrapped->sig;
		for (u8 i = 0;i < wrapped->arg_types.size();i++) {
			signature.arg_types.push_back(mgr->get(wrapped->arg_types[i].name()));
			if (!signature.arg_types[i]) {
				throw bind_exception(format("Arg '%d' of function '%s' is of type '%s' that has not been bound yet", i + 1, name.c_str(), wrapped->arg_types[i].name()));
			}

			vm_register last_a = vm_register(integer(vm_register::a0) - 1);
			vm_register last_f = vm_register(integer(vm_register::f0) - 1);

			for (u8 a = 0;a < signature.arg_locs.size();a++) {
				vm_register l = signature.arg_locs[a];
				if (l >= vm_register::a0 && l <= vm_register::a7) last_a = l;
				else last_f = l;
			}

			if (std::string(wrapped->arg_types[i].name()) == "float") signature.arg_locs.push_back(vm_register(integer(last_f) + 1));
			else signature.arg_locs.push_back(vm_register(integer(last_a) + 1));
		}

		signature.return_type = mgr->get(wrapped->return_type.name());
		if (!signature.return_type) {
			throw bind_exception(format("Return value of function '%s' is of type '%s' that has not been bound yet", name.c_str(), wrapped->return_type.name()));
		}

		signature.is_thiscall = wrapped->name.find_first_of(':') != std::string::npos;
		signature.return_loc = vm_register::v0;
		access.wrapped = wrapped;
		mgr->m_ctx->add(this);
	}

	void vm_function::arg(vm_type* type) {
		if (is_host) throw bind_exception("Cannot specify arguments for host functions");
		if (!type) throw bind_exception("No type specified for argument");
		signature.arg_types.push_back(type);
		vm_register last_a = vm_register(integer(vm_register::a0) - 1);
		vm_register last_f = vm_register(integer(vm_register::f0) - 1);

		for (u8 a = 0;a < signature.arg_locs.size();a++) {
			vm_register l = signature.arg_locs[a];
			if (l >= vm_register::a0 && l <= vm_register::a7) last_a = l;
			else last_f = l;
		}

		if (type->name == "decimal") signature.arg_locs.push_back(vm_register(integer(last_f) + 1));
		else signature.arg_locs.push_back(vm_register(integer(last_a) + 1));
	}

	vm_type::vm_type() {
		constructor = nullptr;
		destructor = nullptr;
		size = 0;
		is_primitive = false;
		is_floating_point = false;
		is_unsigned = false;
		is_builtin = false;
		m_wrapped = nullptr;
	}

	vm_type::~vm_type() {
		if (m_wrapped) delete m_wrapped;
	}

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