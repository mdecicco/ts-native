#pragma once
#include <tsn/bind/bind.h>
#include <tsn/bind/bind_func.h>
#include <tsn/bind/ExecutionContext.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Function.h>
#include <tsn/utils/remove_all.h>

#include <utils/String.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        template <typename Ret, typename... Args>
        void __cdecl _func_wrapper(Ret (*func)(Args...), Ret* out, ExecutionContext* ctx, Args... args) {
            if constexpr (std::is_same_v<void, Ret>) {
                func(args...);
            } else {
                new (out) Ret (func(args...));
            }
        }
        
        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(Ret (Cls::*method)(Args...), Ret* out, ExecutionContext* ctx, Cls* self, Args... args) {
            if constexpr (std::is_same_v<void, Ret>) {
                (*self.*method)(args...);
            } else {
                new (out) Ret ((*self.*method)(args...));
            }
        }

        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(Ret (Cls::*method)(Args...) const, Ret* out, ExecutionContext* ctx, Cls* self, Args... args) {
            if constexpr (std::is_same_v<void, Ret>) {
                (*self.*method)(args...);
            } else {
                new (out) Ret ((*self.*method)(args...));
            }
        }

        template <typename Cls, typename... Args>
        void constructor_wrapper(Cls* mem, Args... args) {
            new (mem) Cls (args...);
        }

        template <typename Cls>
        void destructor_wrapper(Cls* obj) {
            obj->~Cls();
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
                            a + 1, fnName, argTpNames[a]
                        ));
                    }

                    if (argNeedsPtr[a] && !argIsPtr[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of function '%s' is an object of type '%s' which is passed by value. "
                            "Passing objects by value is unsupported, please use a pointer or reference type for this argument",
                            a + 1, fnName, argTpNames[a]
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
                    name, type_name<Ret>()
                ));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is a pointer to a pointer, which is not supported at this time",
                    name
                ));
            }

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (*)(Args...), Ret*, ExecutionContext*, Args...) = &_func_wrapper<Ret, Args...>;

            Function* fn = new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + utils::String(selfType ? selfType->getName() + "::" : ""),
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );

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
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (*)(Cls*, Args...), Ret*, ExecutionContext*, Cls*, Args...) = &_func_wrapper<Ret, Cls*, Args...>;

            return new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + selfTp->getName() + "::",
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
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
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionType tmp(retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (Cls::*)(Args...), Ret*, ExecutionContext*, Cls*, Args...) = &_method_wrapper<Cls, Ret, Args...>;

            return new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + selfTp->getName() + "::",
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper),
                0
            );
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
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionType tmp(retTp, args, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (Cls::*)(Args...) const, Ret*, ExecutionContext*, Cls*, Args...) = &_method_wrapper<Cls, Ret, Args...>;

            return new Method(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + selfTp->getName() + "::",
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper),
                0
            );
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
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, voidTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });

            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionType tmp(voidTp, args, false);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*func)(Cls*, Args...) = &constructor_wrapper<Cls, Args...>;
            void (*wrapper)(void (*)(Cls*, Args...), void*, ExecutionContext*, Cls*, Args...) = &_func_wrapper<void, Cls*, Args...>;

            return new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + forType->getName() + "::",
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
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
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, voidTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            
            FunctionType tmp(voidTp, args, false);
            FunctionType* sig = (FunctionType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                treg->addFuncType(sig);
            }

            void (*func)(Cls*) = &destructor_wrapper<Cls>;
            void (*wrapper)(void (*)(Cls*), void*, ExecutionContext*, Cls*) = &_func_wrapper<void, Cls*>;

            return new Function(
                name,
                utils::String(mod ? mod->getName() + "::" : "") + forType->getName() + "::",
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
        }
    };
};