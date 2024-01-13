#pragma once
#include <tsn/bind/bind.h>
#include <tsn/bind/bind_func.h>
#include <tsn/bind/ExecutionContext.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Function.hpp>
#include <tsn/common/types.hpp>
#include <tsn/common/Module.h>
#include <tsn/utils/remove_all.h>

#include <utils/String.h>
#include <utils/Array.hpp>


#ifdef _MSC_VER
#include <xtr1common>
#else
#include <type_traits>
#endif

namespace tsn {
    namespace ffi {
        template <typename Ret, typename... Args>
        void __cdecl _func_wrapper(call_context* ctx, Args... args) {
            Ret (*func)(Args...);
            ((FunctionPointer*)ctx->funcPtr)->get(&func);
            
            ExecutionContext::Push(ctx->ectx);
            if constexpr (std::is_same_v<void, Ret>) {
                func(args...);
            } else {
                if constexpr (std::is_reference_v<Ret>) {
                    using RT = std::remove_reference_t<Ret>;
                    *((RT**)ctx->retPtr) = &func(args...);
                } else {
                    new ((Ret*)(ctx->retPtr)) Ret (func(args...));
                }
            }
            ExecutionContext::Pop();
        }

        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _pseudo_method_wrapper(call_context* ctx, Args... args) {
            Ret (*func)(Cls*, Args...);
            ((FunctionPointer*)ctx->funcPtr)->get(&func);
            
            ExecutionContext::Push(ctx->ectx);
            if constexpr (std::is_same_v<void, Ret>) {
                func(args...);
            } else {
                if constexpr (std::is_reference_v<Ret>) {
                    using RT = std::remove_reference_t<Ret>;
                    *((RT**)ctx->retPtr) = &func((Cls*)ctx->thisPtr, args...);
                } else {
                    new ((Ret*)(ctx->retPtr)) Ret (func((Cls*)ctx->thisPtr, args...));
                }
            }
            ExecutionContext::Pop();
        }
        
        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(call_context* ctx, Args... args) {
            typedef Ret (Cls::*MethodTp)(Args...);
            MethodTp method;
            ((FunctionPointer*)ctx->funcPtr)->get(&method);

            ExecutionContext::Push(ctx->ectx);
            if constexpr (std::is_same_v<void, Ret>) {
                (*((Cls*)ctx->thisPtr).*method)(args...);
            } else {
                if constexpr (std::is_reference_v<Ret>) {
                    using RT = std::remove_reference_t<Ret>;
                    *((RT**)ctx->retPtr) = &(*((Cls*)ctx->thisPtr).*method)(args...);
                } else {
                    new ((Ret*)(ctx->retPtr)) Ret ((*((Cls*)ctx->thisPtr).*method)(args...));
                }
            }
            ExecutionContext::Pop();
        }

        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _const_method_wrapper(call_context* ctx, Args... args) {
            typedef Ret (Cls::*MethodTp)(Args...) const;
            MethodTp method;
            ((FunctionPointer*)ctx->funcPtr)->get(&method);

            ExecutionContext::Push(ctx->ectx);
            if constexpr (std::is_same_v<void, Ret>) {
                (*((Cls*)ctx->thisPtr).*method)(args...);
            } else {
                if constexpr (std::is_reference_v<Ret>) {
                    using RT = std::remove_reference_t<Ret>;
                    *((RT**)ctx->retPtr) = &(*((Cls*)ctx->thisPtr).*method)(args...);
                } else {
                    new ((Ret*)(ctx->retPtr)) Ret ((*((Cls*)ctx->thisPtr).*method)(args...));
                }
            }
            ExecutionContext::Pop();
        }

        template <typename Cls, typename... Args>
        void __cdecl _constructor_wrapper(call_context* ctx, Args... args) {
            ExecutionContext::Push(ctx->ectx);
            new ((Cls*)ctx->thisPtr) Cls (args...);
            ExecutionContext::Pop();
        }

        template <typename Cls>
        void __cdecl _destructor_wrapper(call_context* ctx) {
            ExecutionContext::Push(ctx->ectx);
            ((Cls*)ctx->thisPtr)->~Cls();
            ExecutionContext::Pop();
        }


        template <typename... Args>
        bool validateAndGetArgs(DataTypeRegistry* reg, utils::Array<function_argument>& args, const utils::String& fnName) {
            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
            if constexpr (argc == 0) return true;

            if constexpr (argc > 0) {
                DataType* argTypes[] = { reg->getType<typename remove_all<Args>::type>()... };
                const char* argTpNames[] = { type_name<Args>()... };
                constexpr bool argIsPtr[] = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                constexpr bool argNeedsPtr[] = { !std::is_fundamental_v<Args>... };

                DataType* voidt = reg->getType<void>();
                DataType* voidp = reg->getType<void*>();

                for (u8 a = 0;a < argc;a++) {
                    if (!argTypes[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of function '%s' is of type '%s', which has not been bound",
                            a + 1, fnName.c_str(), argTpNames[a]
                        ));
                    }

                    if (argNeedsPtr[a] && !argIsPtr[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of function '%s' is an object of type '%s' which is passed by value. "
                            "Passing objects by value is unsupported, please use a pointer or reference type for this argument",
                            a + 1, fnName.c_str(), argTpNames[a]
                        ));
                    }

                    bool isVoidPtr = false;
                    if (argIsPtr[a] && argTypes[a]->getId() == voidt->getId()) {
                        isVoidPtr = true;
                    }

                    args.push({
                        (!isVoidPtr && (argNeedsPtr[a] || argIsPtr[a])) ? arg_type::pointer : arg_type::value,
                        isVoidPtr ? voidp : argTypes[a]
                    });
                }
            }

            return true;
        }

        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Args... args), access_modifier access, DataType* selfType) {
            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is '%s', which has not been bound",
                    name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is a pointer to a pointer, which is not supported at this time",
                    name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(selfType, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + utils::String(selfType ? selfType->getName() + "::" : ""),
                sig,
                access,
                func,
                _func_wrapper<Ret, Args...>,
                mod
            );

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access, DataType* selfType) {
            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is '%s', which has not been bound",
                    name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is a pointer to a pointer, which is not supported at this time",
                    name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(selfType, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + utils::String(selfType ? selfType->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                nullptr,
                mod
            );

            fn->makeInline(genFn);

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Cls*, Args...), access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Primitive type of pseudo-class method '%s::%s' is '%s', which has not been bound",
                    type_name<Cls>(), name.c_str(), type_name<Cls>()
                ));
            }

            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of pseudo-class method '%s::%s' is '%s', which has not been bound",
                    selfTp->getName().c_str(), name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of pseudo-class method '%s::%s' is a pointer to a pointer, which is not supported at this time",
                    selfTp->getName().c_str(), name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(selfTp, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _pseudo_method_wrapper<Cls, Ret, Args...>,
                mod
            );

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Primitive type of pseudo-class method '%s::%s' is '%s', which has not been bound",
                    type_name<Cls>(), name.c_str(), type_name<Cls>()
                ));
            }

            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of pseudo-class method '%s::%s' is '%s', which has not been bound",
                    selfTp->getName().c_str(), name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of pseudo-class method '%s::%s' is a pointer to a pointer, which is not supported at this time",
                    selfTp->getName().c_str(), name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(selfTp, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                nullptr,
                mod
            );

            fn->makeInline(genFn);

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...), access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of class method '%s::%s' is '%s', which has not been bound",
                    type_name<Cls>(), name.c_str(), type_name<Cls>()
                ));
            }

            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is '%s', which has not been bound",
                    selfTp->getName().c_str(), name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is a pointer to a pointer, which is not supported at this time",
                    selfTp->getName().c_str(), name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionType tmp(selfTp, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _method_wrapper<Cls, Ret, Args...>,
                0,
                mod
            );
            
            freg->registerFunction(m);

            return m;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...) const, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of class method '%s::%s' is '%s', which has not been bound",
                    type_name<Cls>(), name.c_str(), type_name<Cls>()
                ));
            }

            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is '%s', which has not been bound",
                    selfTp->getName().c_str(), name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is a pointer to a pointer, which is not supported at this time",
                    selfTp->getName().c_str(), name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionType tmp(selfTp, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _const_method_wrapper<Cls, Ret, Args...>,
                0,
                mod
            );
            
            freg->registerFunction(m);

            return m;
        }

        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of class method '%s::%s' is '%s', which has not been bound",
                    type_name<Cls>(), name.c_str(), type_name<Cls>()
                ));
            }

            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is '%s', which has not been bound",
                    selfTp->getName().c_str(), name.c_str(), type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of class method '%s::%s' is a pointer to a pointer, which is not supported at this time",
                    selfTp->getName().c_str(), name.c_str()
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionType tmp(selfTp, retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                nullptr,
                0,
                mod
            );

            m->makeInline(genFn);
            
            freg->registerFunction(m);

            return m;
        }

        template <typename Cls, typename... Args>
        Function* bind_constructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of constructor '%s::constructor' is '%s', which has not been bound",
                    type_name<Cls>(), type_name<Cls>()
                ));
            }

            utils::String name = "constructor";

            DataType* ptrTp = treg->getType<void*>();
            DataType* voidTp = treg->getType<void>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });

            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(selfTp, voidTp, args, false);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                _constructor_wrapper<Cls, Args...>,
                mod
            );

            freg->registerFunction(fn);

            return fn;
        }

        template <typename Cls>
        Function* bind_destructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of destructor '%s::destructor' is '%s', which has not been bound",
                    type_name<Cls>(), type_name<Cls>()
                ));
            }

            utils::String name = "destructor";

            DataType* ptrTp = treg->getType<void*>();
            DataType* voidTp = treg->getType<void>();

            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, ptrTp });
            
            FunctionType tmp(selfTp, voidTp, args, false);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                _destructor_wrapper<Cls>,
                mod
            );

            freg->registerFunction(fn);

            return fn;
        }
    };
};