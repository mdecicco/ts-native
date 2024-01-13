#pragma once
#include <tsn/bind/call_common.hpp>
#include <tsn/bind/ffi_argument.hpp>
#include <tsn/ffi/Function.h>
#include <tsn/interfaces/IBackend.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>

namespace tsn {
    namespace ffi {
        template <typename ...Args>
        void call_hostToScript(Context* ctx, Function* f, void* result, void* self, Args&&... args) {
            if (ctx->getConfig()->disableExecution) return;
            
            backend::IBackend* be = ctx->getBackend();
            if (!be) return;
            
            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            // + 1 because argc can be 0
            void* ptrBuf[argc + 1] = { 0 };

            const auto* sigArgs = f->getSignature()->getArguments().data();

            u8 pbi = 0;
            void* argBuf[] = {
                getArg<Args>(std::forward<Args>(args), &ptrBuf[pbi], sigArgs[pbi + 1].argType, pbi)...,
                nullptr // because argc can be 0 and argBuf can't have length 0
            };

            ExecutionContext exec(ctx);
            call_context cctx;
            cctx.ectx = &exec;
            cctx.funcPtr = nullptr;
            cctx.retPtr = result;
            cctx.thisPtr = self;
            cctx.capturePtr = nullptr;
            
            be->call(f, &cctx, result, argBuf);
        }
    };
};
