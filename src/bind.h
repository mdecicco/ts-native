#pragma once
#include <any>
#include <vector>
#include <typeindex>
#include <tuple>
#include <string>

#include <asmjit/asmjit.h>

#include <types.h>
#include <robin_hood.h>

#define fimm(f) asmjit::imm(reinterpret_cast<void*>(f))
namespace gjs {
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
			wrapped_function(std::type_index ret, std::vector<std::type_index> args) : return_type(ret), arg_types(args) { }

			// class methods must receive /this/ pointer as first argument
			virtual std::any call(const arg_arr& args) = 0;
			
			std::type_index return_type;
			std::vector<std::type_index> arg_types;
		};

		namespace x86 {
			using compiler = asmjit::x86::Compiler;
			using reg = asmjit::x86::Gp;

			template <typename param_type>
			u8 compile_get_parameter(compiler& c, reg& arg_vec, reg* out_params, size_t pidx) {
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
			void compile_get_parameters(compiler& c, reg& in_args, reg* out_params, index_list<Indices...>) {
				// Trick the compiler into calling compile_get_parameter for each argument type/index,
				// passing the type and index to the function
				std::vector<u8> dummy = {
					compile_get_parameter<Args>(
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

				global_function(asmjit::JitRuntime& rt, func_type f) : wrapped_function(typeid(Ret), { typeid(Args)... }), original_func(f) {
					asmjit::CodeHolder h;
					h.init(rt.codeInfo());
					compiler c(&h);
					c.addFunc(asmjit::FuncSignatureT<Ret, const arg_arr&>(asmjit::CallConv::kIdHost));
					reg in_args = c.newGp(asmjit::Type::IdOfT<const arg_arr&>::kTypeId);
					c.setArg(0, in_args);

					std::vector<reg> params = { c.newGp(asmjit::Type::IdOfT<Args>::kTypeId)... };
					compile_get_parameters<Args...>(c, in_args, params.data(), index_range<0, sizeof...(Args)>());

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

					class_method(asmjit::JitRuntime& rt, method_type f) : wrapped_function(typeid(Ret), { typeid(Args)... }), original_func(call_class_method<Ret, Cls, Args...>) {
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
						compile_get_parameters<Cls*, Args...>(c, in_args, params.data(), index_range<0, sizeof...(Args) + 1>());
			
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
		wrapped_function* wrap(asmjit::JitRuntime& rt, Ret(*func)(Args...)) {
			return new x86::global_function<Ret, Args...>(rt, func);
		};

		template <typename Ret, typename Cls, typename... Args>
		wrapped_function* wrap(asmjit::JitRuntime& rt, Ret(Cls::*func)(Args...)) {
			return new x86::class_method<Ret, Cls, Args...>(rt, func);
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
		wrapped_function* wrap_constructor(asmjit::JitRuntime& rt) {
			return wrap(rt, construct_object<Cls, Args...>);
		}

		template <typename Cls>
		wrapped_function* wrap_destructor(asmjit::JitRuntime& rt) {
			return wrap(rt, destruct_object<Cls>);
		}


		enum property_flags {
			pf_none				= 0b00000000,
			pf_read_only		= 0b00000001,
			pf_write_only		= 0b00000010
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

			wrapped_class(asmjit::JitRuntime& _rt) : rt(_rt) { }

			asmjit::JitRuntime& rt;
			std::vector<wrapped_function*> constructors;
			robin_hood::unordered_map<std::string, wrapped_function*> methods;
			robin_hood::unordered_map<std::string, property*> properties;
			wrapped_function* destructor;
		};

		template <typename Cls>
		struct wrap_class : wrapped_class {
			wrap_class(asmjit::JitRuntime& _rt) : wrapped_class(_rt) { }

			template <typename... Args>
			wrap_class& constructor() {
				constructors.push_back(wrap_constructor<Cls, Args...>(rt));
				if (!destructor) destructor = wrap_destructor<Cls>(rt);
				return *this;
			}

			template <typename Ret, typename... Args>
			wrap_class& method(const std::string& name, Ret(Cls::*func)(Args...)) {
				methods[name] = wrap(rt, func);
				return *this;
			}

			template <typename T>
			wrap_class& prop(const std::string& name, T Cls::*member, u8 flags = property_flags::pf_none) {
				if (properties.find(name) != properties.end()) {
					// exception
					return *this;
				}
				u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
				properties[name] = new property(nullptr, nullptr, typeid(T), offset, flags);
				return *this;
			}

			template <typename T>
			wrap_class& prop(const std::string& name, T(Cls::*getter)(), T(Cls::*setter)(T) = nullptr, u8 flags = property_flags::pf_none) {
				if (properties.find(name) != properties.end()) {
					// exception
					return *this;
				}
				properties[name] = new property(
					wrap(rt, getter),
					setter ? wrap(rt, setter) : nullptr,
					typeid(T),
					0,
					flags
				);
				return *this;
			}
		};
	};
};
#undef fimm