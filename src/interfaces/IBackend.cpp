#include <tsn/interfaces/IBackend.h>

namespace tsn {
    namespace backend {
        IBackend::IBackend(Context* ctx) : IContextual(ctx) {
        }

        IBackend::~IBackend() {
        }

        void IBackend::beforeCompile(Pipeline* input) {
        }
    };
};