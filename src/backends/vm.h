#include <pipeline.h>

namespace gjs {
    class vm_context;
    class vm_backend : public backend {
        public:
            vm_backend(vm_context* ctx);
            ~vm_backend();

            virtual void generate(const ir_code& ir);
    };
};