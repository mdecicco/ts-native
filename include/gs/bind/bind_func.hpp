#pragma once
#include <gs/bind/bind.h>
#include <gs/bind/bind_func.h>
#include <gs/bind/ExecutionContext.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/Function.h>
#include <gs/utils/remove_all.h>

#include <utils/String.h>
#include <utils/Array.hpp>

namespace gs {
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
                DataType* argTypes[] = { reg->getType<remove_all<Args>::type>()... };
                const char* argTpNames[] = { type_name<Args>()... };
                constexpr bool argIsPtr[] = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                constexpr bool argNeedsPtr[] = { !std::is_fundamental_v<Args>... };

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

                    args.push({
                        (argNeedsPtr[a] || argIsPtr[a]) ? arg_type::pointer : arg_type::value,
                        argTypes[a]
                    });
                }
            }

            return true;
        }

        template <typename Ret, typename... Args>
        Function* bind_function(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Args... args), access_modifier access) {
            DataType* retTp = treg->getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format(
                    "Return type of function '%s' is '%s', which has not been bound",
                    name, type_name<Ret>()
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

            FunctionSignatureType tmp(retTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (*)(Args...), Ret*, ExecutionContext*, Args...) = &_func_wrapper<Ret, Args...>;

            return new Function(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Cls*, Args...), access_modifier access) {
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

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            if (!validateAndGetArgs<Args...>(treg, args, name)) {
                return nullptr;
            }

            FunctionSignatureType tmp(retTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (*)(Cls*, Args...), Ret*, ExecutionContext*, Cls*, Args...) = &_func_wrapper<Ret, Cls*, Args...>;

            return new Function(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...), access_modifier access) {
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

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionSignatureType tmp(retTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (Cls::*)(Args...), Ret*, ExecutionContext*, Cls*, Args...) = &_method_wrapper<Cls, Ret, Args...>;

            return new Method(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper),
                0
            );
        }
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...) const, access_modifier access) {
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

            DataType* ptrTp = treg->getType<void*>();

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            validateAndGetArgs<Args...>(treg, args, name);

            FunctionSignatureType tmp(retTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*wrapper)(Ret (Cls::*)(Args...) const, Ret*, ExecutionContext*, Cls*, Args...) = &_method_wrapper<Cls, Ret, Args...>;

            return new Method(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper),
                0
            );
        }

        template <typename Cls, typename... Args>
        Function* bind_constructor(FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of constructor '%s::constructor' is '%s', which has not been bound",
                    type_name<Cls>(), type_name<Cls>()
                ));
            }

            utils::String name = forType->getName() + "::constructor";

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

            FunctionSignatureType tmp(voidTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*func)(Cls*, Args...) = &constructor_wrapper<Cls, Args...>;
            void (*wrapper)(void (*)(Cls*, Args...), void*, ExecutionContext*, Cls*, Args...) = &_func_wrapper<void, Cls*, Args...>;

            return new Function(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
        }

        template <typename Cls>
        Function* bind_destructor(FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access) {
            DataType* selfTp = treg->getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format(
                    "Class type of destructor '%s::destructor' is '%s', which has not been bound",
                    type_name<Cls>(), type_name<Cls>()
                ));
            }

            utils::String name = forType->getName() + "::destructor";

            DataType* ptrTp = treg->getType<void*>();
            DataType* voidTp = treg->getType<void>();

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, voidTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, selfTp });
            
            FunctionSignatureType tmp(voidTp, args);
            FunctionSignatureType* sig = (FunctionSignatureType*)treg->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionSignatureType(tmp);
                treg->addFuncType(sig);
            }

            void (*func)(Cls*) = &destructor_wrapper<Cls>;
            void (*wrapper)(void (*)(Cls*), void*, ExecutionContext*, Cls*) = &_func_wrapper<void, Cls*>;

            return new Function(
                name,
                sig,
                access,
                *reinterpret_cast<void**>(&func),
                *reinterpret_cast<void**>(&wrapper)
            );
        }
    };
};