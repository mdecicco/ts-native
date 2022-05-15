#include <gs/utils/function_match.h>
#include <gs/common/DataType.h>
#include <gs/common/Function.h>

#include <utils/String.h>
#include <utils/Array.hpp>

namespace gs {
    utils::Array<ffi::Function*> function_match(
        const utils::String& name,
        ffi::DataType* retTp,
        ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    ) {
        return funcs.filter([&name, retTp, argTps, argCount, flags](ffi::Function* fn) {
            // Order checks by performance cost
            ffi::FunctionSignatureType* sig = fn->getSignature();

            // strict return check
            if (retTp && (flags & fm_strict_return) && sig->getReturnType()->getId() != retTp->getId()) return false;

            const auto& args = sig->getArguments();

            // arg count check (including implicit)
            if (!(flags & fm_skip_implicit_args) && argCount != args.size()) return false;
            
            // function name match
            if (fn->getName() != name) return false;

            // arg count check (excluding implicit)
            if (flags & fm_skip_implicit_args) {
                u8 implicitCount = 0;
                args.some([&implicitCount](const ffi::function_argument& a, u32 idx) {
                    implicitCount = (u8)idx;
                    return !a.isImplicit();
                });

                if (argCount != (args.size() - implicitCount)) return false;
            }

            // strict arg type check
            if (flags & fm_strict_args) {
                u8 argIdx = 0;
                bool argsMatch = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                    if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                    if (a.dataType->getId() != argTps[argIdx++]->getId()) return true;
                    return false;
                });

                if (!argsMatch) return false;
            }

            // flexible return type check
            if (retTp && !(flags & fm_strict_return) && !sig->getReturnType()->isConvertibleTo(retTp)) return false;
            
            // flexible arg type check
            if (!(flags & fm_strict_args)) {
                u8 argIdx = 0;
                bool argsMatch = !args.some([argTps, argCount, flags, &argIdx](const ffi::function_argument& a) {
                    if ((flags & fm_skip_implicit_args) && a.isImplicit()) return false;
                    if (!a.dataType->isConvertibleTo(argTps[argIdx++])) return true;
                    return false;
                });

                if (!argsMatch) return false;
            }

            return true;
        });
    }
};