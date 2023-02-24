#pragma once
#include <tsn/bind/call_common.hpp>
#include <tsn/bind/ffi_argument.hpp>
#include <tsn/ffi/Function.h>
#include <tsn/interfaces/IBackend.h>
#include <tsn/common/Context.h>

namespace tsn {
    namespace ffi {
        template <typename ...Args>
        void call_hostToScript(Context* ctx, Function* f, void* result, Args&&... args) {
            backend::IBackend* be = ctx->getBackend();
            if (!be) return;
            
            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;

            // + 1 because argc can be 0
            void* ptrBuf[argc + 1] = { 0 };
            u8 pbi = 0;

            const auto* sigArgs = f->getSignature()->getArguments().data();

            void* argBuf[] = {
                getArg<Args>(std::forward<Args>(args), &ptrBuf[pbi], sigArgs[3 + pbi++].argType)...,
                nullptr // because argc can be 0 and argBuf can't have length 0
            };

            ExecutionContext exec(ctx);
            be->call(f, &exec, result, argBuf);
        }
    };
};
