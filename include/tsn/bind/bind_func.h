#pragma once
#include <tsn/common/types.h>
#include <xtr1common>

namespace utils {
    class String;
};

namespace tsn {
    namespace ffi {
        class Function;
        class DataType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class ExecutionContext;

        template <typename Ret, typename... Args>
        void __cdecl _func_wrapper(Ret (*func)(Args... args), Ret* out, ExecutionContext* ctx, Args... args);
        
        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(Ret (Cls::*func)(Args... args), Ret* out, ExecutionContext* ctx, Cls* self, Args... args);

        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(Ret (Cls::*func)(Args... args) const, Ret* out, ExecutionContext* ctx, Cls* self, Args... args);



        template <typename Ret, typename... Args>
        Function* bind_function(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Args...), access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Cls*, Args...), access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...), access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...) const, access_modifier access);

        template <typename Cls, typename... Args>
        Function* bind_constructor(FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access);

        template <typename Cls>
        Function* bind_destructor(FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access);
    };
};