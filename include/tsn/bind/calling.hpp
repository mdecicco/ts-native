#pragma once
#include <tsn/ffi/Object.h>
#include <tsn/bind/call_host_to_host.hpp>
#include <tsn/bind/call_host_to_script.hpp>

namespace tsn {
    class Context;

    namespace ffi {
        template <typename ...Args>
        Object call(Context* ctx, Function* f, Args&&... args) {
            Object ret = Object(ctx, f->getSignature()->getReturnType());

            if (f->getWrapperAddress().isValid()) {
                call_hostToHost(ctx, f, ret.getPtr(), nullptr, args...);
            } else {
                call_hostToScript(ctx, f, ret.getPtr(), nullptr, args...);
            }

            return ret;
        }

        template <typename ...Args>
        void call(Context* ctx, void* ret, Function* f, Args&&... args) {
            if (f->getWrapperAddress().isValid()) {
                call_hostToHost(ctx, f, ret, nullptr, args...);
            } else {
                call_hostToScript(ctx, f, ret, nullptr, args...);
            }
        }
        
        template <typename ...Args>
        Object call_method(Context* ctx, Function* f, void* self, Args&&... args) {
            Object ret = Object(ctx, f->getSignature()->getReturnType());

            if (f->getWrapperAddress().isValid()) {
                call_hostToHost(ctx, f, ret.getPtr(), self, args...);
            } else {
                call_hostToScript(ctx, f, ret.getPtr(), self, args...);
            }

            return ret;
        }

        template <typename ...Args>
        void call_method(Context* ctx, void* ret, Function* f, void* self, Args&&... args) {
            if (f->getWrapperAddress().isValid()) {
                call_hostToHost(ctx, f, ret, self, args...);
            } else {
                call_hostToScript(ctx, f, ret, self, args...);
            }
        }
    };
};