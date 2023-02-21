#include <tsn/vm/VMBackend.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/backend/RegisterAllocationStep.h>

namespace tsn {
    namespace vm {
        Backend::Backend(Context* ctx) : IBackend(ctx) {
        }

        Backend::~Backend() {
        }
        
        void Backend::beforeCompile(Pipeline* input) {
            input->addOptimizationStep(new backend::RegisterAllocatonStep(m_ctx, 16, 16));
        }

        void Backend::generate(Pipeline* input) {
        }

        void Backend::call(ffi::Function* func, ffi::ExecutionContext* ctx, void* retPtr, void** args) {
        }
    };
};