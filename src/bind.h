#pragma once
#include <any>
#include <vector>
#include <typeindex>
#include <tuple>
#include <string>

#include <asmjit/asmjit.h>

#include <types.h>
#include <robin_hood.h>

#define declare_input_binding(type, ctx_name, in_name, reg_name) template<> void to_reg<type>(vm_context* ctx_name, const type& in_name, u64* reg_name)
#define declare_output_binding(type, ctx_name, out_name, reg_name) template<> void from_reg<type>(vm_context* ctx_name, type* out_name, u64* reg_name)

#define fimm(f) asmjit::imm(reinterpret_cast<void*>(f))
namespace gjs {
	class vm_context;
	enum class vm_register;
	class vm_function;
	class vm_type;

	class bind_exception : public std::exception {
		public:
			bind_exception(const std::string& _text) : text(_text) { }
			~bind_exception() { }

			virtual char const* what() const { return text.c_str(); }

			std::string text;
	};

	namespace bind {
		struct wrapped_class;
	};


	template<class T> struct remove_all { typedef T type; };
	template<class T> struct remove_all<T*> : remove_all<T> {};
	template<class T> struct remove_all<T&> : remove_all<T> {};
	template<class T> struct remove_all<T&&> : remove_all<T> {};
	template<class T> struct remove_all<T const> : remove_all<T> {};
	template<class T> struct remove_all<T volatile> : remove_all<T> {};
	template<class T> struct remove_all<T const volatile> : remove_all<T> {};
	template<class T> struct remove_all<T[]> : remove_all<T> {};

	template <typename T>
	inline const char* base_type_name() {
		return typeid(remove_all<T>::type).name();
	}

	class type_manager {
		public:
			type_manager(vm_context* ctx);
			~type_manager();

			vm_type* get(const std::string& internal_name);

			template <typename T>
			vm_type* get() {
				return get(base_type_name<T>());
			}

			vm_type* add(const std::string& name, const std::string& internal_name);

			void finalize_class(bind::wrapped_class* wrapped);

			std::vector<vm_type*> all();

		protected:
			friend class vm_function;
			vm_context* m_ctx;
			robin_hood::unordered_map<std::string, vm_type*> m_types;
	};

	namespace bind {
		using arg_arr = std::vector<std::any>;

		// basic array lookup, probably could be replaced
		// by clever work with asmjit to call vector::data
		// directly
		std::any& get_input_param(arg_arr& params, u32 idx);

		// The structure that encapsulates index lists
		template <size_t... Is>
		struct index_list { };

		// Primary template for index range builder
		template <size_t MIN, size_t N, size_t... Is>
		struct range_builder;

		// Base step
		template <size_t MIN, size_t... Is>
		struct range_builder<MIN, MIN, Is...> {
			typedef index_list<Is...> type;
		};

		// Induction step
		template <size_t MIN, size_t N, size_t... Is>
		struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...> { };

		// Meta-function that returns a [MIN, MAX) index range
		template <size_t MIN, size_t MAX>
		using index_range = typename range_builder<MIN, MAX>::type;

		struct wrapped_function {
			wrapped_function(std::type_index ret, std::vector<std::type_index> args, const std::string& _name, const std::string& _sig)
			: return_type(ret), arg_types(args), name(_name), sig(_sig) { }

			// class methods must receive /this/ pointer as first argument
			virtual std::any call(const arg_arr& args) = 0;
			
			std::string name;
			std::string sig;
			std::type_index return_type;
			std::vector<std::type_index> arg_types;
			u64 address;
		};

		namespace x86  {
			using compiler = asmjit::x86::Compiler;
			using reg = asmjit::x86::Gp;

			template <typename param_type>
			u8 compile_get_parameter(type_manager* tpm, const std::string& fname, compiler& c, reg& arg_vec, reg* out_params, size_t pidx) {
				if (!tpm->get<param_type>()) {
					throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<param_type>(), fname.c_str()));
				}

				reg cur_param = c.newGp(asmjit::Type::IdOfT<std::any&>::kTypeId);

				// cur_param = get_input_param(arg_vec, pidx)
				auto index_call = c.call(
					fimm(get_input_param), 
					asmjit::FuncSignatureT<std::any&, arg_arr&, u32>(asmjit::CallConv::kIdHost)
				);
				index_call->setArg(0, arg_vec);
				index_call->setArg(1, asmjit::imm(pidx));
				index_call->setRet(0, cur_param);

				// param_value = std::any_cast<param_type>(cur_param)
				auto cast_call = c.call(
					fimm((param_type(*)(std::any&))std::any_cast<param_type>),
					asmjit::FuncSignatureT<param_type, std::any&>(asmjit::CallConv::kIdHost)
				);
				cast_call->setArg(0, cur_param);
				cast_call->setRet(0, out_params[pidx]);
				return true;
			}

			// Build ASM code to translate vector<std::any> into the actual parameters
			// for a function call
			template <typename... Args, size_t... Indices>
			void compile_get_parameters(type_manager* tpm, const std::string& fname, compiler& c, reg& in_args, reg* out_params, index_list<Indices...>) {
				// Trick the compiler into calling compile_get_parameter for each argument type/index,
				// passing the type and index to the function
				std::vector<u8> dummy = {
					compile_get_parameter<Args>(
						tpm,
						fname,
						c,
						in_args,
						out_params,
						Indices
					)...
				};
			}
		
			// Call class method on object
			// Only visible to class methods with non-void return
			template <typename Ret, typename Cls, typename... Args>
			typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
			call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
				return (*self.*method)(args...);
			}

			// Call class method on object
			// Only visible to class methods with void return
			template <typename Ret, typename Cls, typename... Args>
			typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
			call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
				(*self.*method)(args...);
			}

			template <typename Ret, typename... Args>
			struct global_function : wrapped_function {
				typedef Ret (*func_type)(Args...);
				typedef std::tuple_size<std::tuple<Args...>> arg_count;

				global_function(type_manager* tpm, asmjit::JitRuntime& rt, func_type f, const std::string& name) : wrapped_function(typeid(remove_all<Ret>::type), { typeid(remove_all<Args>::type)... }, name, typeid(f).name()), original_func(f) {
					if (!tpm->get<Ret>()) {
						throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
					}
					address = u64(reinterpret_cast<void*>(f));
					
					asmjit::CodeHolder h;
					h.init(rt.codeInfo());
					compiler c(&h);
					c.addFunc(asmjit::FuncSignatureT<Ret, const arg_arr&>(asmjit::CallConv::kIdHost));
					reg in_args = c.newGp(asmjit::Type::IdOfT<const arg_arr&>::kTypeId);
					c.setArg(0, in_args);

					std::vector<reg> params = { c.newGp(asmjit::Type::IdOfT<Args>::kTypeId)... };
					compile_get_parameters<Args...>(tpm, name, c, in_args, params.data(), index_range<0, sizeof...(Args)>());

					if (std::is_void<Ret>::value) {
						auto fcall = c.call(fimm(f), asmjit::FuncSignatureT<Ret, Args...>(asmjit::CallConv::kIdHost));
						for (u8 i = 0;i < arg_count::value;i++) fcall->setArg(i, params[i]);
						c.ret();
					} else {
						reg ret = c.newGp(asmjit::Type::IdOfT<Ret>::kTypeId);
						auto fcall = c.call(fimm(f), asmjit::FuncSignatureT<Ret, Args...>(asmjit::CallConv::kIdHost));
						for (u8 i = 0;i < arg_count::value;i++) fcall->setArg(i, params[i]);
						fcall->setRet(0, ret);
						c.ret(ret);
					}
					c.endFunc();
					c.finalize();
					rt.add(&func, &h);
				}

				template <typename T = Ret>
				typename std::enable_if<!std::is_same<T, void>::value, std::any>::type
				operator()(const arg_arr& args) {
					return func(args);
				}

				template <typename T = Ret>
				typename std::enable_if<std::is_same<T, void>::value, std::any>::type
				operator()(const arg_arr& args) {
					func(args);
					return std::any();
				}

				virtual std::any call(const arg_arr& args) {
					return (*this)(args);
				}

				func_type original_func;
				Ret (*func)(const arg_arr&);
			};

			template <typename Ret, typename Cls, typename... Args>
			struct class_method : wrapped_function {
				public:
					typedef Ret (Cls::*method_type)(Args...);
					typedef Ret (*func_type)(method_type, Cls*, Args...);
					typedef std::tuple_size<std::tuple<Args...>> ac;

					class_method(type_manager* tpm, asmjit::JitRuntime& rt, method_type f, const std::string& name) : wrapped_function(typeid(remove_all<Ret>::type), { typeid(remove_all<Args>::type)... }, name, typeid(f).name()), original_func(call_class_method<Ret, Cls, Args...>) {
						if (!tpm->get<Cls>()) {
							throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
						}
						if (!tpm->get<Ret>()) {
							throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
						}
						address = u64(reinterpret_cast<void*>(&f));

						asmjit::CodeHolder h;
						h.init(rt.codeInfo());
						compiler c(&h);

						asmjit::FuncSignatureBuilder fs;
						fs.addArg(asmjit::Type::kIdIntPtr);
						fs.addArgT<Cls*>();

						std::vector<u32> argTypes = { asmjit::Type::IdOfT<Args>::kTypeId... };
						for (u8 i = 0;i < ac::value;i++) fs.addArg(argTypes[i]);
						fs.setCallConv(asmjit::CallConv::kIdHost);
						fs.setRetT<Ret>();

						c.addFunc(asmjit::FuncSignatureT<Ret, const arg_arr&>(asmjit::CallConv::kIdHost));
						reg in_args = c.newGp(asmjit::Type::IdOfT<const arg_arr&>::kTypeId);
						c.setArg(0, in_args);

						std::vector<reg> params = { c.newGp(asmjit::Type::IdOfT<Args>::kTypeId)... };
						params.insert(params.begin(), c.newGp(asmjit::Type::IdOfT<Cls*>::kTypeId));
						compile_get_parameters<Cls*, Args...>(tpm, name, c, in_args, params.data(), index_range<0, sizeof...(Args) + 1>());
			
						if (std::is_void<Ret>::value) {
							auto call = c.call(asmjit::imm(original_func), fs);
							call->setArg(0, asmjit::imm(*reinterpret_cast<intptr_t*>(&f)));
							for (u8 i = 0;i <= ac::value;i++) call->setArg(i + 1, params[i]);
							c.ret();
						} else {
							reg ret = c.newGp(asmjit::Type::IdOfT<Ret>::kTypeId);
							auto call = c.call(asmjit::imm(original_func), fs);
							call->setArg(0, asmjit::imm(*reinterpret_cast<intptr_t*>(&f)));
							for (u8 i = 0;i <= ac::value;i++) call->setArg(i + 1, params[i]);
							call->setRet(0, ret);
							c.ret(ret);

						}
						c.endFunc();
						c.finalize();
						rt.add(&func, &h);
					}

					template <typename T = Ret>
					typename std::enable_if<!std::is_same<T, void>::value, std::any>::type
					operator()(const arg_arr& args) {
						return func(args);
					}

					template <typename T = Ret>
					typename std::enable_if<std::is_same<T, void>::value, std::any>::type
					operator()(const arg_arr& args) {
						func(args);
						return std::any();
					}

					virtual std::any call(const arg_arr& args) {
						return (*this)(args);
					}

					func_type original_func;
					Ret (*func)(const arg_arr&);
			};
		};

		template <typename Ret, typename... Args>
		wrapped_function* wrap(type_manager* tpm, asmjit::JitRuntime& rt, const std::string& name, Ret(*func)(Args...)) {
			return new x86::global_function<Ret, Args...>(tpm, rt, func, name);
		};

		template <typename Ret, typename Cls, typename... Args>
		wrapped_function* wrap(type_manager* tpm, asmjit::JitRuntime& rt, const std::string& name, Ret(Cls::*func)(Args...)) {
			return new x86::class_method<Ret, Cls, Args...>(tpm, rt, func, name);
		};

		template <typename Cls, typename... Args>
		Cls* construct_object(Args... args) {
			return new Cls(args...);
		}

		template <typename Cls>
		void destruct_object(Cls* obj) {
			delete obj;
		}

		template <typename Cls, typename... Args>
		wrapped_function* wrap_constructor(type_manager* tpm, asmjit::JitRuntime& rt) {
			return wrap(tpm, rt, format("%s::construct", typeid(remove_all<Cls>::type).name()), construct_object<Cls, Args...>);
		}

		template <typename Cls>
		wrapped_function* wrap_destructor(type_manager* tpm, asmjit::JitRuntime& rt) {
			return wrap(tpm, rt, format("%s::destruct", typeid(remove_all<Cls>::type).name()), destruct_object<Cls>);
		}

		enum property_flags {
			pf_none				= 0b00000000,
			pf_read_only		= 0b00000001,
			pf_write_only		= 0b00000010,
			pf_object_pointer	= 0b00000100
		};

		struct wrapped_class {
			struct property {
				property(wrapped_function* g, wrapped_function* s, std::type_index t, u32 o, u8 f) :
					getter(g), setter(s), type(t), offset(o), flags(f) { }

				wrapped_function* getter;
				wrapped_function* setter;
				std::type_index type;
				u32 offset;
				u8 flags;
			};

			wrapped_class(asmjit::JitRuntime& _rt, const std::string& _name, const std::string& _internal_name, size_t _size) :
				rt(_rt), name(_name), internal_name(_internal_name), size(_size), ctor(nullptr), dtor(nullptr)
			{
			}

			asmjit::JitRuntime& rt;
			std::string name;
			std::string internal_name;
			wrapped_function* ctor;
			robin_hood::unordered_map<std::string, wrapped_function*> methods;
			robin_hood::unordered_map<std::string, property*> properties;
			wrapped_function* dtor;
			size_t size;
		};

		template <typename Cls>
		struct wrap_class : wrapped_class {
			wrap_class(type_manager* tpm, asmjit::JitRuntime& _rt, const std::string& name) : wrapped_class(_rt, name, typeid(remove_all<Cls>::type).name(), sizeof(remove_all<Cls>::type)), types(tpm) {
				tpm->add(name, typeid(remove_all<Cls>::type).name());
			}

			template <typename... Args>
			wrap_class& constructor() {
				ctor = wrap_constructor<Cls, Args...>(types, rt);
				if (!dtor) dtor = wrap_destructor<Cls>(types, rt);
				return *this;
			}

			template <typename Ret, typename... Args>
			wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...)) {
				methods[_name] = wrap(types, rt, name + "::" + _name, func);
				return *this;
			}

			template <typename T>
			wrap_class& prop(const std::string& _name, T Cls::*member, u8 flags = property_flags::pf_none) {
				if (properties.find(_name) != properties.end()) {
					throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
				}

				if (!types->get<T>()) {
					throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
				}

				u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
				properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), offset, flags);
				return *this;
			}

			template <typename T>
			wrap_class& prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T) = nullptr, u8 flags = property_flags::pf_none) {
				if (properties.find(_name) != properties.end()) {
					throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
				}

				if (!types->get<T>()) {
					throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
				}

				properties[_name] = new property(
					wrap(rt, name + "::get_" + _name, getter),
					setter ? wrap(rt, name + "::set_" + _name, setter) : nullptr,
					typeid(remove_all<T>::type),
					0,
					flags
				);
				return *this;
			}

			void finalize() {
				types->finalize_class(this);
			}

			type_manager* types;
		};

		enum class value_type {
			integer,
			decimal,
			string,
			object
		};

		struct script_function {
			std::string name;
			vm_context* ctx;
			address entry;
			value_type ret_type;
			vm_register ret_location;
			std::vector<value_type> arg_types;
			std::vector<vm_register> arg_locs;

			template <typename T, typename... Args>
			void call(T* result, Args... args);
		};


		template <typename T>
		void to_reg(vm_context* context, const T& in, u64* reg_ptr);

		template <typename T>
		void from_reg(vm_context* context, T* out, u64* reg_ptr);

		template <typename T>
		void set_argument(script_function* func, u8 idx, const T& arg) {
			vm_state* state = func->ctx->state();
			to_reg(func->ctx, arg, &state->registers[(integer)func->arg_locs[idx]]);
		}

		declare_input_binding(integer, context, in, reg_ptr);
		declare_output_binding(integer, context, out, reg_ptr);
		declare_input_binding(decimal, context, in, reg_ptr);
		declare_output_binding(decimal, context, out, reg_ptr);

		template <typename Arg, typename... Args>
		void set_arguments(script_function* func, u8 idx, const Arg& a, Args... rest) {
			set_argument(func, idx, a);

			if (sizeof...(rest) == 0) return;
			set_arguments(func, idx + 1, rest...);
		}
		void set_arguments(script_function* func, u8 idx);
	};

	class vm_function {
		public:
			vm_function(const std::string name, address addr);
			vm_function(type_manager* mgr, bind::wrapped_function* wrapped);

			void arg(vm_type* type);

			std::string name;
			bool is_host;

			struct {
				std::string text;
				vm_type* return_type;
				vm_register return_loc;
				std::vector<vm_type*> arg_types;
				std::vector<vm_register> arg_locs;
				bool is_thiscall;

			} signature;

			union {
				bind::wrapped_function* wrapped;
				address entry;
			} access;
	};

	class vm_type {
		public:
			std::string name;
			std::string internal_name;
			u32 size;
			bool is_primitive;

			struct property {
				u8 flags;
				std::string name;
				vm_type* type;
				u32 offset;
				vm_function* getter;
				vm_function* setter;
			};

			enum class operator_func {
				add,
				sub,
				mul,
				div,
				eq
				// ...
			};

			std::vector<property> properties;
			vm_function* constructor;
			vm_function* destructor;
			std::vector<vm_function*> methods;
			robin_hood::unordered_map<operator_func, vm_function*> operators;
			robin_hood::unordered_map<vm_type*, vm_function*> converters;

		protected:
			friend class type_manager;
			vm_type();
	};
};
#undef fimm