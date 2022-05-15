#pragma once
#include <gs/common/Object.h>
#include <gs/bind/call_host_to_host.hpp>
#include <gs/bind/call_host_to_script.hpp>

namespace gs {
    class Context;

    namespace ffi {
        template <typename ...Args>
        Object call(Context* ctx, Function* f, Args&&... args) {
            Object ret = Object(ctx, f->getSignature()->getReturnType());

            if (f->getWrapperAddress()) {
                call_hostToHost(f, ret.getPtr(), args...);
            } else {
                call_hostToScript(f, ret.getPtr(), args...);
            }

            return ret;
        }

        template <typename ...Args>
        void call(void* ret, Function* f, Args&&... args) {
            if (f->getWrapperAddress()) {
                call_hostToHost(f, ret, args...);
            } else {
                call_hostToScript(f, ret, args...);
            }
        }
        
        template <typename ...Args>
        Object call_method(Context* ctx, Function* f, void* self, Args&&... args) {
            Object ret = Object(ctx, f->getSignature()->getReturnType());

            if (f->getWrapperAddress()) {
                call_hostToHost(f, ret.getPtr(), self, args...);
            } else {
                call_hostToScript(f, ret.getPtr(), self, args...);
            }

            return ret;
        }

        template <typename ...Args>
        void call_method(void* ret, Function* f, void* self, Args&&... args) {
            if (f->getWrapperAddress()) {
                call_hostToHost(f, ret, self, args...);
            } else {
                call_hostToScript(f, ret, self, args...);
            }
        }
    };
};