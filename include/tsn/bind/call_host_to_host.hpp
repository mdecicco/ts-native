#pragma once
#include <tsn/bind/call_common.hpp>
#include <tsn/bind/ffi_argument.hpp>
#include <tsn/ffi/Function.hpp>
#include <tsn/bind/ExecutionContext.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>

namespace tsn {
    namespace ffi {
        template <typename ...Args>
        void call_hostToHost(Context* ctx, Function* f, void* result, void* self, Args&&... args) {
            if (ctx->getConfig()->disableExecution) return;

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
            void (*fptr)(call_context*, Args...);
            f->getWrapperAddress().get(&fptr);

            call_context cctx;
            cctx.ectx = nullptr;
            cctx.funcPtr = &f->getAddress();
            cctx.retPtr = result;
            cctx.thisPtr = self;
            cctx.capturePtr = nullptr;
            
            void* pcctx = &cctx;

            // + 1 because argc can be 0
            void* ptrBuf[argc + 1] = { 0 };

            const auto* sigArgs = f->getSignature()->getArguments().data();

            u8 pbi = 0;
            void* argBuf[] = {
                &pcctx,
                getArg<Args>(std::forward<Args>(args), &ptrBuf[pbi], sigArgs[pbi + 1].argType, pbi)...
            };
            
            pbi = 0;
            ffi_type* typeBuf[] = {
                &ffi_type_pointer,
                (sigArgs[(pbi++) + 1].argType == arg_type::pointer ? &ffi_type_pointer : getType<Args>(args))...
            };
            
            ffi_cif cif;
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc + 1, &ffi_type_void, typeBuf) != FFI_OK) {
                return;
            }

            ExecutionContext exec(ctx);
            cctx.ectx = &exec;
            ffi_call(&cif, reinterpret_cast<void (*)()>(fptr), result, argBuf);
        }
    };
};
