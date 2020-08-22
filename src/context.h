#pragma once
#include <robin_hood.h>
#include <string>
#include <any>
#include <vm.h>

namespace gjs {
	namespace bind {
		struct wrapped_function;
		struct wrapped_class;
	};

	enum class value_type {
		integer,
		decimal,
		string,
		object
	};

	struct vec3 { float x,y,z; };

	class vm_context;
	struct script_function {
		vm_context* ctx;
		address entry;
		value_type ret_type;
		vm_register ret_location;
		std::vector<value_type> arg_types;
		std::vector<vm_register> arg_locs;

		template <typename T, typename... Args>
		void call(T* result, Args... args);
	};

	struct script_class {
	};

	class vm_context {
		public:
			vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size);
			~vm_context();

			void add(const std::string& name, bind::wrapped_function* func);
			void add(const std::string& name, bind::wrapped_class* cls);
			void add(const std::string& name, script_function* func);

			bind::wrapped_function* host_function(const std::string& name);
			bind::wrapped_class* host_prototype(const std::string& name);
			script_function* function(const std::string& name);

			inline vm_state* state() { return &m_vm.state; }
			inline instruction_array* code() { return &m_instructions; }

			void execute(address entry);


		protected:
			robin_hood::unordered_map<std::string, bind::wrapped_function*> m_host_functions;
			robin_hood::unordered_map<std::string, script_function*> m_script_functions;
			robin_hood::unordered_map<std::string, bind::wrapped_class*> m_host_classes;
			vm m_vm;
			instruction_array m_instructions;
	};

	template <typename T>
	void to_reg(const T& in, u64* reg_ptr) {
	}

	template <>
	void to_reg<integer>(const integer& in, u64* reg_ptr);

	template <typename Arg, typename... Args>
	void set_arguments(script_function* func, u8 idx, const Arg& a, Args... rest) {
		vm_state* state = func->ctx->state();

		switch (func->arg_types[idx]) {
			case value_type::integer: {
				break;
			}
			case value_type::decimal: {
				break;
			}
			case value_type::string: {
				break;
			}
			case value_type::object: {
				break;
			}
		}

		to_reg<Arg>(a, &state->registers[(integer)func->arg_locs[idx]]);

		if (sizeof...(rest) == 0) return;
		set_arguments(func, idx + 1, rest...);
	}
	void set_arguments(script_function* func, u8 idx);



	template <typename T>
	void from_reg(T* out, u64* reg_ptr) {
	}

	template <>
	void from_reg<integer>(integer* out, u64* reg_ptr);

	template <typename T, typename... Args>
	void script_function::call(T* result, Args... args) {
		if (sizeof...(args) != arg_locs.size()) {
			// exception
		}

		if (arg_locs.size() > 0) set_arguments(this, 0, args...);
		ctx->execute(entry);

		vm_state* state = ctx->state();

		switch (ret_type) {
			case value_type::integer: {
				if (!std::is_integral_v<T>) {
					// exception
				}
				break;
			}
			case value_type::decimal: {
				if (!std::is_floating_point_v<T>) {
					// exception
				}
				break;
			}
			case value_type::string: {
				if (!std::is_same_v<T, char*>) {
					// exception
				}
				break;
			}
			case value_type::object: {
				if (!std::is_reference_v<T> && !std::is_pointer_v<T>) {
					// exception
				}
				break;
			}
		}

		from_reg(result, &state->registers[(integer)ret_location]);
	}
};
