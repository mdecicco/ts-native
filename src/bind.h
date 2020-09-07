#pragma once
#include <types.h>
#include <builtin.h>
#include <template_utils.hpp>

#include <vector>
#include <typeindex>
#include <tuple>
#include <string>

#include <asmjit/asmjit.h>
#include <robin_hood.h>


#define declare_input_binding(type, ctx_name, in_name, reg_name) template<> void to_reg<type>(vm_context* ctx_name, const type& in_name, u64* reg_name)
#define declare_output_binding(type, ctx_name, out_name, reg_name) template<> void from_reg<type>(vm_context* ctx_name, type* out_name, u64* reg_name)

#define fimm(f) asmjit::imm(reinterpret_cast<void*>(f))
namespace gjs {
    class vm_context;
    class vm_function;
    class vm_type;
    class type_manager;
    enum class vm_register;

    class bind_exception : public std::exception {
        public:
            bind_exception(const std::string& _text) : text(_text) { }
            ~bind_exception() { }

            virtual char const* what() const { return text.c_str(); }

            std::string text;
    };

    namespace bind {
        struct wrapped_function {
            wrapped_function(std::type_index ret, std::vector<std::type_index> args, const std::string& _name, const std::string& _sig)
            : return_type(ret), arg_types(args), name(_name), sig(_sig), is_static_method(false), address(0), ret_is_ptr(false) { }

            // class methods must receive /this/ pointer as first argument
            virtual void call(void* ret, void** args) = 0;
            
            std::string name;
            std::string sig;
            std::type_index return_type;
            bool ret_is_ptr;
            std::vector<std::type_index> arg_types;
            std::vector<bool> arg_is_ptr;
            bool is_static_method;
            u64 address;
        };

        struct wrapped_class {
            struct property {
                property(wrapped_function* g, wrapped_function* s, std::type_index t, u64 o, u8 f) :
                    getter(g), setter(s), type(t), offset(o), flags(f) { }

                wrapped_function* getter;
                wrapped_function* setter;
                std::type_index type;
                u64 offset;
                u8 flags;
            };

            wrapped_class(asmjit::JitRuntime& _rt, const std::string& _name, const std::string& _internal_name, size_t _size) :
                rt(_rt), name(_name), internal_name(_internal_name), size(_size), ctor(nullptr), dtor(nullptr), requires_subtype(false)
            {
            }

            ~wrapped_class();

            asmjit::JitRuntime& rt;
            std::string name;
            std::string internal_name;
            bool requires_subtype;
            wrapped_function* ctor;
            robin_hood::unordered_map<std::string, wrapped_function*> methods;
            robin_hood::unordered_map<std::string, property*> properties;
            wrapped_function* dtor;
            size_t size;
        };


        /*
         * Magic for wrapping functions and methods with ASM
         * so that they can be called with runtime generated
         * argument lists (x86 version, thanks to asmjit)
         */
        namespace x86  {
            using compiler = asmjit::x86::Compiler;
            using reg = asmjit::x86::Gp;
        
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

            template <typename T>
            asmjit::BaseReg reg_for_type(compiler& c) {
                if constexpr (std::is_floating_point_v<T>) return c.newXmm();
                else return c.newGp(asmjit::Type::IdOfT<T>::kTypeId);
            }

            template <typename Ret, typename... Args>
            struct global_function : wrapped_function {
                typedef Ret (*func_type)(Args...);
                typedef std::tuple_size<std::tuple<Args...>> arg_count;

                global_function(type_manager* tpm, asmjit::JitRuntime& rt, func_type f, const std::string& name) :
                    wrapped_function(typeid(remove_all<Ret>::type),
                    { typeid(remove_all<Args>::type)... },
                    name,
                    typeid(f).name()),
                    original_func(f)
                {
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
                    }
                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    address = u64(reinterpret_cast<void*>(f));
                    
                    // begin generating the wrapper function
                    asmjit::CodeHolder h;
                    h.init(rt.codeInfo());
                    compiler c(&h);

                    // define function signature 'void wrapped(void* out_result, void** args)'
                    c.addFunc(asmjit::FuncSignatureT<void, void*, void**>(asmjit::CallConv::kIdHost));

                    // bind out_result to register
                    reg out = c.newGp(asmjit::Type::IdOfT<Ret*>::kTypeId);
                    c.setArg(0, out);

                    // bind args to register
                    reg in_args = c.newGp(asmjit::Type::IdOfT<void**>::kTypeId);
                    c.setArg(1, in_args);

                    // generate list of registers to bind each argument to
                    asmjit::BaseReg params[] = { reg_for_type<Args>(c)... };

                    // void** arg_ptr = args;
                    reg temp = c.newGp(asmjit::Type::IdOfT<void**>::kTypeId);
                    c.mov(temp, in_args);

                    // params[i] <- arg_ptr[i]
                    for (u8 i = 0;i < arg_count::value;i++) {
                        if (!arg_is_ptr[i]) {
                            if (arg_types[i] == std::type_index(typeid(f32))) {
                                c.movss(static_cast<asmjit::x86::Xmm&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                            } else if (arg_types[i] == std::type_index(typeid(f64))) {
                                c.movsd(static_cast<asmjit::x86::Xmm&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                            } else {
                                c.mov(static_cast<asmjit::x86::Gp&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                            }
                        } else c.mov(static_cast<asmjit::x86::Gp&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                    }

                    // original_func(...)
                    auto fcall = c.call(fimm(f), asmjit::FuncSignatureT<Ret, Args...>(asmjit::CallConv::kIdHost));
                    
                    // specify each argument
                    for (u8 i = 0;i < arg_count::value;i++) fcall->setArg(i, params[i]);

                    // maybe get return value
                    if constexpr (!std::is_void<Ret>::value) {
                        if (std::is_floating_point_v<Ret>) {
                            // ret = original_func(...)
                            asmjit::x86::Xmm ret = c.newXmm();
                            fcall->setRet(0, ret);

                            if (sizeof(Ret) == 4) {
                                //*((f32*)out_result) = ret
                                c.movss(asmjit::x86::ptr(out), ret);
                            } else if (sizeof(Ret) == 8) {
                                //*((f64*)out_result) = ret
                                c.movsd(asmjit::x86::ptr(out), ret);
                            } else {
                                //*((long f64*)out_result) = ret
                                c.movdqu(asmjit::x86::ptr(out), ret);
                            }
                        } else {
                            // ret = original_func(...)
                            reg ret = c.newGp(asmjit::Type::IdOfT<Ret>::kTypeId);
                            fcall->setRet(0, ret);

                            // *out_result = ret
                            c.mov(asmjit::x86::ptr(out), ret);
                        }
                    }

                    // return
                    c.ret();

                    // generate
                    c.endFunc();
                    c.finalize();

                    // assign func to the wrapper function
                    rt.add(&func, &h);
                }

                virtual void call(void* ret, void** args) {
                    if (!func) {
                        throw runtime_exception("Attempted to call improperly bound function");
                    }
                    func(ret, args);
                }

                func_type original_func;
                void (*func)(void*, void**);
            };

            template <typename Ret, typename Cls, typename... Args>
            struct class_method : wrapped_function {
                public:
                    typedef Ret (Cls::*method_type)(Args...);
                    typedef Ret (*func_type)(method_type, Cls*, Args...);
                    typedef std::tuple_size<std::tuple<Args...>> ac;

                    class_method(type_manager* tpm, asmjit::JitRuntime& rt, method_type f, const std::string& name) :
                        wrapped_function(
                            typeid(remove_all<Ret>::type),
                            { typeid(remove_all<Cls>::type), typeid(remove_all<Args>::type)... },
                            name,
                            typeid(f).name()
                        ), original_func(call_class_method<Ret, Cls, Args...>)
                    {
                        if (!tpm->get<Cls>()) {
                            throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                        }
                        if (!tpm->get<Ret>()) {
                            throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                        }

                        // describe the function for the wrapped_function interface
                        ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                        arg_is_ptr = { true, (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                        address = *(u64*)reinterpret_cast<void*>(&f);

                        // begin generating the wrapper function
                        asmjit::CodeHolder h;
                        h.init(rt.codeInfo());
                        compiler c(&h);

                        // define function signature of the call_class_method function
                        asmjit::FuncSignatureBuilder fs;
                        fs.addArg(asmjit::Type::kIdIntPtr); // method pointer
                        fs.addArgT<Cls*>(); // self

                        // the rest of the argument types
                        std::vector<u32> argTypes = { asmjit::Type::IdOfT<Args>::kTypeId... };
                        for (u8 i = 0;i < ac::value;i++) fs.addArg(argTypes[i]);

                        // call convention and return type
                        fs.setCallConv(asmjit::CallConv::kIdHost);
                        fs.setRetT<Ret>();

                        // define function signature 'void wrapped(void* out_result, void** args)'
                        c.addFunc(asmjit::FuncSignatureT<void, void*, void**>(asmjit::CallConv::kIdHost));

                        // bind out_result to register
                        reg out = c.newGp(asmjit::Type::IdOfT<Ret*>::kTypeId);
                        c.setArg(0, out);

                        // bind args to register
                        reg in_args = c.newGp(asmjit::Type::IdOfT<void**>::kTypeId);
                        c.setArg(1, in_args);

                        // generate list of registers to bind each argument to
                        asmjit::BaseReg params[] = {
                            // this_obj
                            c.newGp(asmjit::Type::IdOfT<Cls*>::kTypeId),
                            // the other args
                            reg_for_type<Args>(c)...
                        };

                        // void** arg_ptr = args;
                        reg temp = c.newGp(asmjit::Type::IdOfT<void**>::kTypeId);
                        c.mov(temp, in_args);

                        // params[i] <- arg_ptr[i]
                        for (u8 i = 0;i < ac::value + 1;i++) {
                            if (!arg_is_ptr[i]) {
                                if (arg_types[i] == std::type_index(typeid(f32))) {
                                    c.movss(static_cast<asmjit::x86::Xmm&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                                } else if (arg_types[i] == std::type_index(typeid(f64))) {
                                    c.movsd(static_cast<asmjit::x86::Xmm&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                                } else {
                                    c.mov(static_cast<asmjit::x86::Gp&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                                }
                            } else c.mov(static_cast<asmjit::x86::Gp&>(params[i]), asmjit::x86::Mem(temp, i * sizeof(void*)));
                        }

                        // call_class_method<Ret, Cls, Args...>(...)
                        auto fcall = c.call(asmjit::imm(original_func), fs);

                        // specify each argument
                        fcall->setArg(0, asmjit::imm(*reinterpret_cast<intptr_t*>(&f))); // method ptr
                        for (u8 i = 0;i <= ac::value;i++) fcall->setArg(i + 1, params[i]);

                        // maybe get return value
                        if constexpr (!std::is_void<Ret>::value) {
                            if (std::is_floating_point_v<Ret>) {
                                // ret = original_func(...)
                                asmjit::x86::Xmm ret = c.newXmm();
                                fcall->setRet(0, ret);

                                if (sizeof(Ret) == 4) {
                                    //*((f32*)out_result) = ret
                                    c.movss(asmjit::x86::ptr(out), ret);
                                } else if (sizeof(Ret) == 8) {
                                    //*((f64*)out_result) = ret
                                    c.movsd(asmjit::x86::ptr(out), ret);
                                } else {
                                    //*((long f64*)out_result) = ret
                                    c.movdqu(asmjit::x86::ptr(out), ret);
                                }
                            } else {
                                // ret = original_func(...)
                                reg ret = c.newGp(asmjit::Type::IdOfT<Ret>::kTypeId);
                                fcall->setRet(0, ret);

                                // *out_result = ret
                                c.mov(asmjit::x86::ptr(out), ret);
                            }
                        }

                        // return
                        c.ret();

                        // generate
                        c.endFunc();
                        c.finalize();

                        // assign func to the wrapper function
                        rt.add(&func, &h);
                    }

                    virtual void call(void* ret, void** args) {
                        if (!func) {
                            throw runtime_exception("Attempted to call improperly bound function");
                        }
                        return func(ret, args);
                    }

                    func_type original_func;
                    void (*func)(void*, void**);
            };
        };

        /*
         * Function wrapping helper
         */
        template <typename Ret, typename... Args>
        wrapped_function* wrap(type_manager* tpm, asmjit::JitRuntime& rt, const std::string& name, Ret(*func)(Args...)) {
            return new x86::global_function<Ret, Args...>(tpm, rt, func, name);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, asmjit::JitRuntime& rt, const std::string& name, Ret(Cls::*func)(Args...)) {
            return new x86::class_method<Ret, Cls, Args...>(tpm, rt, func, name);
        };


        /*
         * Class wrapping helper
         */
        template <typename Cls, typename... Args>
        Cls* construct_object(void* mem, Args... args) {
            return new (mem) Cls(args...);
        }

        template <typename Cls>
        void destruct_object(Cls* obj) {
            obj->~Cls();
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
            pf_none             = 0b00000000,
            pf_read_only        = 0b00000001,
            pf_write_only       = 0b00000010,
            pf_pointer          = 0b00000100,
            pf_static           = 0b00001000
        };


        // todo:
        //   - Change constructor/destructor to be stored the
        //     same way as regular methods
        //   - Change the way methods are stored so that they
        //     can be overloaded

        template <typename Cls>
        struct wrap_class : wrapped_class {
            wrap_class(type_manager* tpm, asmjit::JitRuntime& _rt, const std::string& name) : wrapped_class(_rt, name, typeid(remove_all<Cls>::type).name(), sizeof(remove_all<Cls>::type)), types(tpm) {
                vm_type* tp = tpm->add(name, typeid(remove_all<Cls>::type).name());
                tp->is_host = true;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
            wrap_class& constructor() {
                requires_subtype = std::is_same_v<std::tuple_element_t<0, std::tuple<Args...>>, vm_type*>;
                ctor = wrap_constructor<Cls, Args...>(types, rt);
                if (!dtor) dtor = wrap_destructor<Cls>(types, rt);
                return *this;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
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

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(*func)(Args...)) {
                methods[_name] = wrap(types, rt, name + "::" + _name, func);
                methods[_name]->is_static_method = true;
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
            wrap_class& prop(const std::string& _name, T *member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), (u64)member, flags | pf_static);
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, rt, name + "::get_" + _name, getter),
                    wrap(types, rt, name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }


            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, rt, name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            vm_type* finalize() {
                return types->finalize_class(this);
            }

            type_manager* types;
        };



        template <typename T>
        void to_reg(vm_context* context, const T& in, u64* reg_ptr);

        template <typename T>
        void from_reg(vm_context* context, T* out, u64* reg_ptr);

        declare_input_binding(integer, context, in, reg_ptr);
        declare_output_binding(integer, context, out, reg_ptr);
        declare_input_binding(decimal, context, in, reg_ptr);
        declare_output_binding(decimal, context, out, reg_ptr);

        template <typename Arg, typename... Args>
        void set_arguments(vm_context* ctx, vm_function* func, u8 idx, const Arg& a, Args... rest);
        void set_arguments(vm_context* ctx, vm_function* func, u8 idx);

        template <typename T>
        void set_argument(vm_context* ctx, vm_function* func, u8 idx, const T& arg) {
            vm_state* state = ctx->state();
            to_reg(ctx, arg, &state->registers[(integer)func->signature.arg_locs[idx]]);
        }

        template <typename Arg, typename... Args>
        void set_arguments(vm_context* ctx, vm_function* func, u8 idx, const Arg& a, Args... rest) {
            set_argument(ctx, func, idx, a);

            if (sizeof...(rest) == 0) return;
            set_arguments(ctx, func, idx + 1, rest...);
        }
    };
};
#undef fimm