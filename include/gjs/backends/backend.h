#pragma once
#include <gjs/common/pipeline.h>

namespace gjs {
    class backend {
        public:
            backend();
            virtual ~backend() { }

            inline script_context* context() { return m_ctx; }

            /*
            * Takes the final IR code and generates instructions for some target
            * architecture. What it does with that code is out of the scope of
            * the pipeline.
            */
            virtual void generate(compilation_output& in) = 0;

            // number of GP registers (see register_allocator file for details)
            virtual u16 gp_count() const = 0;

            // number of FP registers (see register_allocaor file for details)
            virtual u16 fp_count() const = 0;

            // whether or not to perform the register allocation step
            virtual bool perform_register_allocation() const = 0;

            // called by script_context after it has been constructed
            virtual void init();

            // do any processing necessary on all bindings once there is nothing left
            // to bind
            virtual void commit_bindings() { }

        protected:
            friend class script_context;
            /*
             * Called from the context's function call helper
             *
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
             * References are essentially pointers, so those are valid.
             */
            virtual void call(script_function* func, void* ret, void** args) = 0;

            script_context* m_ctx;
    };
};