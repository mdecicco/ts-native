#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IBackend.h>

namespace tsn {
    namespace vm {
        class Backend : public backend::IBackend {
            public:
                Backend(Context* ctx);
                ~Backend();
                
                virtual void beforeCompile(Pipeline* input);
                virtual void generate(Pipeline* input);
                virtual void call(ffi::Function* func, ffi::ExecutionContext* ctx, void* retPtr, void** args);
        };
    };
};