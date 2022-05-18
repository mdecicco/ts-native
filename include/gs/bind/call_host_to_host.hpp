#pragma once
#include <gs/bind/call_common.hpp>
#include <gs/bind/ffi_argument.hpp>
#include <gs/common/Function.h>
#include <gs/bind/ExecutionContext.h>

namespace gs {
    namespace ffi {
        template <typename ...Args>
        void call_hostToHost(Context* ctx, Function* f, void* result, Args&&... args) {
            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
            void* fptr = f->getAddress();

            void* ectx = nullptr;

            // + 1 because argc can be 0
            void* ptrBuf[argc + 1] = { 0 };
            u8 pbi = 0;

            const auto* sigArgs = f->getSignature()->getArguments().data();

            void* argBuf[] = {
                &fptr,
                &result,
                &ectx,
                getArg<Args>(std::forward<Args>(args), &ptrBuf[pbi], sigArgs[3 + pbi++].argType)...
            };

            ffi_type* typeBuf[] = {
                &ffi_type_pointer, // func->getAddress()
                &ffi_type_pointer, // return value pointer
                &ffi_type_pointer, // execution context
                getType<Args>(args)...
            };
            
            ffi_cif cif;
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc + 3, &ffi_type_void, typeBuf) != FFI_OK) {
                return;
            }

            ExecutionContext exec(ctx);
            ectx = (void*)&exec;
            ffi_call(&cif, reinterpret_cast<void (*)()>(f->getWrapperAddress()), result, argBuf);
        }
    };
};
