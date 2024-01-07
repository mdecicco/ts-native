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
        ffi::Function* fn,
        bool* wasStrictEqual
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
        const utils::String& fName = fn->getName();
        if (fName != name) {
            if (name.size() > 8 && fName.size() > 8 && name[0] == 'o' && fName[0] == 'o' && name[7] == 'r' && fName[7] == 'r') {
                utils::String a = fName;
                utils::String b = name;
                a.replaceAll("operator ", "operator");
                b.replaceAll("operator ", "operator");

                if (a != b) return false;
            } else return false;
        }

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
        bool didCheckArgsStrict = false;
        bool argsStrictEqual = false;
        if (!(flags & fm_ignore_args) && (flags & fm_strict_args)) {
            u8 argIdx = 0;
            bool argsMatch = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                if (!a.dataType->isEqualTo(argTps[argIdx++])) return true;
                return false;
            });

            if (!argsMatch) return false;

            argsStrictEqual = true;
            didCheckArgsStrict = true;
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

        if (wasStrictEqual && argTps) {
            if (retTp && !sig->getReturnType()->isEqualTo(retTp)) *wasStrictEqual = false;
            else if (!didCheckArgsStrict) {
                if (argsStrictEqual) {
                    *wasStrictEqual = true;
                } else {
                    u8 argIdx = 0;
                    *wasStrictEqual = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                        if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                        if (!a.dataType->isEqualTo(argTps[argIdx++])) return true;
                        return false;
                    });
                }
            } else {
                *wasStrictEqual = argsStrictEqual;
            }
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
        ffi::Function* strictMatch = nullptr;
        auto matches = funcs.filter([&name, retTp, argTps, argCount, flags, &strictMatch](ffi::Function* fn) {
            if (!fn || strictMatch) return false;
            bool wasStrict = false;
            bool result = func_match_filter(name, retTp, argTps, argCount, flags, fn, (flags & fm_strict) ? nullptr : &wasStrict);

            if (wasStrict) {
                strictMatch = fn;
                return false;
            }

            return result;
        });

        if (strictMatch) return { strictMatch };

        return matches;
    }

    utils::Array<ffi::Method*> function_match(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Method*>& funcs,
        function_match_flags flags
    ) {
        ffi::Method* strictMatch = nullptr;

        auto matches = funcs.filter([&name, retTp, argTps, argCount, flags, &strictMatch](ffi::Method* fn) {
            if (!fn || strictMatch) return false;
            bool wasStrict = false;
            bool result = func_match_filter(name, retTp, argTps, argCount, flags, fn, (flags & fm_strict) ? nullptr : &wasStrict);

            if (wasStrict) {
                strictMatch = fn;
                return false;
            }

            return result;
        });

        if (strictMatch) return { strictMatch };

        return matches;
    }
};