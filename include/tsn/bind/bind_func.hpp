#pragma once
#include <tsn/bind/bind.h>
#include <tsn/bind/bind_func.h>
#include <tsn/bind/ExecutionContext.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.hpp>
#include <tsn/ffi/Function.hpp>
#include <tsn/common/types.hpp>
#include <tsn/common/Module.h>
#include <tsn/utils/remove_all.h>

#include <utils/String.h>
#include <utils/Array.hpp>

#include <functional>
#include <type_traits>

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



        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Args... args), access_modifier access, DataType* selfType) {
            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + utils::String(selfType ? selfType->getName() + "::" : ""),
                treg->getSignatureType<Ret, Args...>(selfType),
                access,
                func,
                _func_wrapper<Ret, Args...>,
                mod,
                selfType
            );

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access, DataType* selfType) {
            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + utils::String(selfType ? selfType->getName() + "::" : ""),
                treg->getSignatureType<Ret, Args...>(selfType),
                access,
                nullptr,
                nullptr,
                mod,
                selfType
            );

            fn->makeInline(genFn);

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Cls*, Args...), access_modifier access) {
            FunctionType* sig = treg->getMethodSignatureType<Cls, Ret, Args...>();
            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _pseudo_method_wrapper<Cls, Ret, Args...>,
                mod,
                sig->getThisType()
            );

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access) {
            FunctionType* sig = treg->getMethodSignatureType<Cls, Ret, Args...>();
            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                nullptr,
                mod,
                sig->getThisType()
            );

            fn->makeInline(genFn);

            freg->registerFunction(fn);

            return fn;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...), access_modifier access) {
            FunctionType* sig = treg->getMethodSignatureType<Cls, Ret, Args...>();
            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _method_wrapper<Cls, Ret, Args...>,
                0,
                mod,
                sig->getThisType()
            );
            
            freg->registerFunction(m);

            return m;
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...) const, access_modifier access) {
            FunctionType* sig = treg->getMethodSignatureType<Cls, Ret, Args...>();
            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                sig,
                access,
                func,
                _const_method_wrapper<Cls, Ret, Args...>,
                0,
                mod,
                sig->getThisType()
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

            return bind_method<Ret, Args...>(mod, freg, treg, selfTp, name, genFn, access);
        }

        template <typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* selfTp, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access) {
            Method* m = new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : ""),
                treg->getSignatureType<Ret, Args...>(selfTp),
                access,
                nullptr,
                nullptr,
                0,
                mod,
                selfTp
            );

            m->makeInline(genFn);
            
            freg->registerFunction(m);

            return m;
        }


        template <typename Cls, typename... Args>
        Function* bind_constructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            Function* fn = new Function(
                "constructor",
                utils::String(mod ? mod->getName() + "::" : ""),
                treg->getMethodSignatureType<Cls, void, Args...>(),
                access,
                nullptr,
                _constructor_wrapper<Cls, Args...>,
                mod,
                forType
            );

            freg->registerFunction(fn);

            return fn;
        }

        template <typename Cls>
        Function* bind_destructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            Function* fn = new Function(
                "destructor",
                utils::String(mod ? mod->getName() + "::" : ""),
                treg->getMethodSignatureType<Cls, void>(),
                access,
                nullptr,
                _destructor_wrapper<Cls>,
                mod,
                forType
            );

            freg->registerFunction(fn);

            return fn;
        }
    };
};