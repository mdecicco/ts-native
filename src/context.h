#pragma once
#include <robin_hood.h>
#include <string>
#include <vm.h>
#include <bind.h>
#include <source_map.h>
#include <util.h>

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
	class vm_context {
		public:
			vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size);
			~vm_context();

			void add(const std::string& name, bind::wrapped_function* func);
			void add(const std::string& name, bind::wrapped_class* cls);
			void add(const std::string& name, bind::script_function* func);

			bind::wrapped_function* host_function(const std::string& name);
			bind::wrapped_class* host_prototype(const std::string& name);
			bind::script_function* _function(const std::string& name);
			std::vector<bind::wrapped_class*> host_prototypes();
			std::vector<bind::wrapped_function*> host_functions();

			void add(vm_function* func);
			vm_function* function(const std::string& name);
			vm_function* function(u64 address);
			std::vector<vm_function*> all_functions();


			inline vm_state* state() { return &m_vm.state; }
			inline instruction_array* code() { return &m_instructions; }
			inline source_map* map() { return &m_map; }
			inline type_manager* types() { return &m_types; }
			inline asmjit::JitRuntime* jit() { return m_jit; }
			inline bool is_executing() const { return m_is_executing; }
			inline bool log_exceptions() const { return m_catch_exceptions; }
			inline void log_exceptions(bool doLog) { m_catch_exceptions = doLog; }

			void add_code(const std::string& filename, const std::string& code);
			void execute(address entry);


		protected:
			robin_hood::unordered_map<u64, vm_function*> m_funcs_by_addr;
			robin_hood::unordered_map<std::string, vm_function*> m_funcs;

			asmjit::JitRuntime* m_jit;
			type_manager m_types;
			vm m_vm;
			instruction_array m_instructions;
			source_map m_map;
			bool m_is_executing;
			bool m_catch_exceptions;

			// about to be removed
			robin_hood::unordered_map<std::string, bind::wrapped_function*> m_host_functions;
			robin_hood::unordered_map<std::string, bind::script_function*> m_script_functions;
			robin_hood::unordered_map<std::string, bind::wrapped_class*> m_host_classes;
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

		template <typename T, typename... Args>
		void script_function::call(T* result, Args... args) {
			try {
				if (sizeof...(args) != arg_locs.size()) {
					throw runtime_exception(format("Script function '%s' takes %d arguments, %d %s provided", name.c_str(), arg_locs.size(), sizeof...(args), (sizeof...(args)) == 1 ? "was" : "were"));
				}

				if (arg_locs.size() > 0) set_arguments(this, 0, args...);
				ctx->execute(entry);

				vm_state* state = ctx->state();

				switch (ret_type) {
					case value_type::integer: {
						if (!std::is_integral_v<T>) {
							throw runtime_exception(format("Return value of script function '%s' is type 'integer', but non-integral type '%s' was provided", name.c_str(), typeid(T).name()));
						}
						break;
					}
					case value_type::decimal: {
						if (!std::is_floating_point_v<T>) {
							throw runtime_exception(format("Return value of script function '%s' is type 'decimal', but non-floating point type '%s' was provided", name.c_str(), typeid(T).name()));
						}
						break;
					}
					case value_type::string: {
						if (!std::is_same_v<T, char*> && !std::is_same_v<T, std::string>) {
							throw runtime_exception(format("Return value of script function '%s' is type 'string', but non-string type '%s' was provided", name.c_str(), typeid(T).name()));
						}
						break;
					}
					case value_type::object: {
						if (!std::is_pointer_v<T>) {
							// exception
							throw runtime_exception(format("Return value of script function '%s' is type 'object', but non-pointer type '%s' was provided", name.c_str(), typeid(T).name()));
						}
						break;
					}
				}

				from_reg(ctx, result, &state->registers[(integer)ret_location]);
			}  catch (runtime_exception& e) {
				if (!ctx->log_exceptions()) throw e;
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
				if (!ctx->log_exceptions()) throw e;
				printf("Caught exception: %s\n", e.what());
			}
		}
	};
};
