#include <tsn/utils/function_match.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Function.h>

#include <utils/String.h>
#include <utils/Array.hpp>

namespace tsn {
    bool func_match_filter(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        function_match_flags flags,
        ffi::Function* fn
    ) {
        // Order checks by performance cost
        if ((flags & fm_exclude_private) && fn->getAccessModifier() == private_access) return false;

        ffi::FunctionType* sig = fn->getSignature();
        if (!sig) return false;

        // strict return check
        if (retTp && (flags & fm_strict_return) && !retTp->isEqualTo(sig->getReturnType())) return false;

        const auto& args = sig->getArguments();

        // arg count check (including implicit)
        if (!((flags & fm_skip_implicit_args) || (flags & fm_ignore_args)) && argCount != args.size()) return false;
        
        // function name match
        if (fn->getName() != name) return false;

        // arg count check (excluding implicit)
        if (!(flags & fm_ignore_args) && (flags & fm_skip_implicit_args)) {
            u8 implicitCount = 0;
            bool all_implicit = !args.some([&implicitCount](const ffi::function_argument& a, u32 idx) {
                implicitCount = (u8)idx;
                return !a.isImplicit();
            });
            if (all_implicit) implicitCount++;

            if (argCount != (args.size() - implicitCount)) return false;
        }

        // strict arg type check
        if (!(flags & fm_ignore_args) && (flags & fm_strict_args)) {
            u8 argIdx = 0;
            bool argsMatch = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                if (!a.dataType->isEqualTo(argTps[argIdx++])) return true;
                return false;
            });

            if (!argsMatch) return false;
        }

        // flexible return type check
        if (retTp && !(flags & fm_strict_return)) {
            if (sig->getReturnType() && !sig->getReturnType()->isConvertibleTo(retTp)) return false;
            else if (!sig->getReturnType()) return false;
        }
        
        // flexible arg type check
        if (!(flags & fm_ignore_args) && !(flags & fm_strict_args)) {
            u8 argIdx = 0;
            bool argsMatch = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                if (!argTps[argIdx++]->isConvertibleTo(a.dataType)) return true;
                return false;
            });

            if (!argsMatch) return false;
        }

        return true;
    }
    utils::Array<ffi::Function*> function_match(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    ) {
        return funcs.filter([&name, retTp, argTps, argCount, flags](ffi::Function* fn) {
            if (!fn) return false;
            return func_match_filter(name, retTp, argTps, argCount, flags, fn);
        });
    }

    utils::Array<ffi::Method*> function_match(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Method*>& funcs,
        function_match_flags flags
    ) {
        return funcs.filter([&name, retTp, argTps, argCount, flags](ffi::Method* fn) {
            if (!fn) return false;
            return func_match_filter(name, retTp, argTps, argCount, flags, fn);
        });
    }
};