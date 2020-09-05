#pragma once
#include <vm.h>
#include <bind.h>
#include <source_map.h>
#include <util.h>
#include <pipeline.h>

#include <robin_hood.h>
#include <string>

namespace asmjit {
	class JitRuntime;
};

namespace gjs {
	class vm_context;
	class runtime_exception : public std::exception {
		public:
			runtime_exception(vm_context* ctx, const std::string& text);
			runtime_exception(const std::string& text);
			~runtime_exception();

			virtual char const* what() const { return text.c_str(); }

			bool raised_from_script;
			std::string file;
			std::string lineText;
			std::string text;
			u32 line;
			u32 col;
	};

	class type_manager;
	class vm_function;
	class vm_context {
		public:
			vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size);
			~vm_context();

			template <class Cls>
			bind::wrap_class<Cls>& bind(const std::string& name) {
				// as long as wrap_class::finalize is called, this will be deleted when it should be
				return *(new bind::wrap_class<Cls>(m_types, *m_jit, name));
			}

			template <typename Ret, typename... Args>
			void bind(Ret(*func)(Args...), const std::string& name) {
				bind::wrapped_function* w = bind::wrap(m_types, *m_jit, name, func);
				new vm_function(m_types, nullptr, w);
			}

			void add(vm_function* func);
			vm_function* function(const std::string& name);
			vm_function* function(u64 address);
			bool set_function_address(u64 old_addr, u64 new_addr);

			std::vector<vm_function*> all_functions();
			std::vector<vm_type*> all_types();

			inline vm_state*			state		() { return &m_vm.state; }
			inline instruction_array*	code		() { return &m_instructions; }
			inline source_map*			map			() { return &m_map; }
			inline type_manager*		types		() { return m_types; }
			inline gjs::vm*				vm			() { return &m_vm; }
			inline asmjit::JitRuntime*	jit			() { return m_jit; }
			inline vm_allocator*		allocator	() { return m_alloc; }
			inline pipeline*			compiler	() { return &m_pipeline; }

			inline bool is_executing	() const { return m_is_executing; }
			inline bool log_instructions() const { return m_log_instructions; }
			inline void log_instructions(bool doLog) { m_log_instructions = doLog; }

			bool add_code(const std::string& filename, const std::string& code);
			void execute(address entry);

		protected:
			robin_hood::unordered_map<u64, vm_function*> m_funcs_by_addr;
			robin_hood::unordered_map<std::string, vm_function*> m_funcs;

			asmjit::JitRuntime* m_jit;
			type_manager* m_types;
			gjs::vm m_vm;
			instruction_array m_instructions;
			source_map m_map;
			vm_allocator* m_alloc;
			pipeline m_pipeline;
			bool m_is_executing;
			bool m_log_instructions;
	};

	namespace bind {
		template <class T>
		void to_reg(vm_context* context, T* const& in, u64* reg_ptr) {
			*reg_ptr = reinterpret_cast<u64>(in);
		}

		template <class T>
		void from_reg(vm_context* context, T** out, u64* reg_ptr) {
			*out = reinterpret_cast<T*>(*reg_ptr);
		}

		template <typename T>
		void to_reg(vm_context* context, const T& in, u64* reg_ptr) {
			throw runtime_exception(context, format("No binding exists for storing type '%s' in a register", typeid(T).name()));
		}

		template <typename T>
		void from_reg(vm_context* context, T* out, u64* reg_ptr) {
			throw runtime_exception(context, format("No binding exists for retrieving type '%s' from a register", typeid(T).name()));
		}
	};
};
