#pragma once
#include <tsn/common/types.h>

namespace utils {
    class String;
};

namespace tsn {
    namespace compiler {
        struct InlineCodeGenContext;
        typedef void (*InlineCodeGenFunc)(compiler::InlineCodeGenContext* ctx);
    };

    namespace ffi {
        class Function;
        class FunctionType;
        class DataType;
        class FunctionRegistry;
        class DataTypeRegistry;
        class ExecutionContext;

        template <typename Ret, typename... Args>
        void __cdecl _func_wrapper(call_context* ctx, Args... args);
        
        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _pseudo_method_wrapper(call_context* ctx, Args... args);
        
        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _method_wrapper(call_context* ctx, Args... args);

        template <typename Cls, typename Ret, typename... Args>
        void __cdecl _const_method_wrapper(call_context* ctx, Args... args);



        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Args...), access_modifier access, DataType* selfType);
        
        template <typename Ret, typename... Args>
        Function* bind_function(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access, DataType* selfType);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (*func)(Cls*, Args...), access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_pseudo_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...), access_modifier access);
        
        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, Ret (Cls::*func)(Args...) const, access_modifier access);

        template <typename Cls, typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access);

        template <typename Ret, typename... Args>
        Function* bind_method(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* selfTp, const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access);

        template <typename Cls, typename... Args>
        Function* bind_constructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access);

        template <typename Cls>
        Function* bind_destructor(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* forType, access_modifier access);
    };
};