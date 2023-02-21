#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>

namespace tsn {
    namespace ffi {
        class Function;
        class ExecutionContext;
    };

    class Pipeline;

    namespace backend {
        class IBackend : public IContextual {
            public:
                IBackend(Context* ctx);
                virtual ~IBackend();

                /**
                 * @brief Called before a pipeline attempts to compile any script. This
                 * function can be used to add any backend-specific optimization or IR
                 * processing steps to the pipeline.
                 * 
                 * @note This function will be called even for cached scripts, before
                 * they are loaded and processed. Though, when a cached script is loaded
                 * it will not have any optimizers run on it, as that already happened.
                 * 
                 * @param input The pipeline that's about to compile a script
                 */
                virtual void beforeCompile(Pipeline* input);

                /**
                 * @brief
                 * Takes the final IR code and generates instructions for some target
                 * architecture. What it does with that code is out of the scope of
                 * the pipeline.
                 */
                virtual void generate(Pipeline* input) = 0;

                /**
                 * @brief
                 * This function is responsible for passing the arguments in the
                 * correct way to the function, whether it be a script function
                 * or a host function. It also needs to take the return value of
                 * the function and store it in the memory referred to by ret.
                 *
                 * For non-pointer arguments:
                 * The value of the argument /is/ the argument. For example, if
                 * a function takes an int32 as the first parameter, the value
                 * of args[0] is not a pointer to an int32, it IS an int32.
                 *
                 * Passing non-pointer arguments that are larger than the size
                 * of a pointer is unsupported.
                 *
                 * References are pointers, so those are valid.
                 */
                virtual void call(ffi::Function* func, ffi::ExecutionContext* ctx, void* retPtr, void** args) = 0;
        };
    };
};