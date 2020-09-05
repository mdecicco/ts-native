#pragma once
#include <types.h>
#include <string>
#include <vector>

namespace gjs {
	class vm_context;
	class type_manager;
	class vm_type;
	enum class vm_register;

	namespace bind {
		struct wrapped_function;
	};

	class vm_function {
		public:
			vm_function(vm_context* ctx, const std::string name, address addr);
			vm_function(type_manager* mgr, vm_type* tp, bind::wrapped_function* wrapped, bool is_ctor = false, bool is_dtor = false);

			void arg(vm_type* type);

			std::string name;
			bool is_host;
			vm_type* is_method_of;

			struct {
				std::string text;
				vm_type* return_type;
				bool returns_on_stack;
				vm_register return_loc;
				std::vector<vm_type*> arg_types;
				std::vector<vm_register> arg_locs;
				bool is_thiscall;
				bool returns_pointer;
				bool is_subtype_obj_ctor;
			} signature;

			union {
				bind::wrapped_function* wrapped;
				address entry;
			} access;

			template <typename Ret, typename... Args>
			void call(Ret* result, Args... args) {
				if (sizeof...(args) != signature.arg_locs.size()) {
					throw runtime_exception(format(
						"Function '%s' takes %d arguments, %d %s provided",
						name.c_str(),
						signature.arg_locs.size(),
						sizeof...(args),
						(sizeof...(args)) == 1 ? "was" : "were"
					));
				}

				if (signature.arg_locs.size() > 0) bind::set_arguments(m_ctx, this, 0, args...);
				if (is_host) m_ctx->vm()->call_external(access.wrapped->address);
				else m_ctx->vm()->execute(*m_ctx->code(), access.entry);

				if (signature.return_type->size != 0) {
					bind::from_reg(m_ctx, result, &m_ctx->state()->registers[(u8)signature.return_loc]);
				}
			}

		protected:
			vm_context* m_ctx;
	};
};

