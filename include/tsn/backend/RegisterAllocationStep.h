#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/interfaces/IOptimizationStep.h>

#include <list>

namespace utils {
    template <typename T>
    class Array;
};

namespace tsn {
    namespace compiler {
        class CodeHolder;
    };

    namespace backend {
        /**
         * @brief
         * Modifies the IR code to only use a specific amount of
         * GP registers and a specific amount of FP registers.
         * 
         * - If the FP register count is set to 0, GP registers will
         *   be used for floating point values
         * - This will insert IR code to store values in the stack
         *   when no registers are available and one is needed
         * - This will insert IR code to load values from the stack
         *   that were stored as a result of this process (when needed)
         * - The resulting code will use virtual registers 0 to gpN
         *   to for GP registers, and virtual registers 0 to fpN for
         *   floating point registers. Backend code generators will
         *   need to map these virtual register IDs to physical registers
         *   using the operand types to determine whether a virtual
         *   reister is GP or FP.
         */
        class RegisterAllocatonStep : public optimize::IOptimizationStep {
            public:
                struct lifetime {
                    compiler::vreg_id reg_id;
                    compiler::vreg_id new_id;
                    compiler::alloc_id stack_loc;
                    u32 begin;
                    u32 end;
                    bool is_fp;

                    bool isConcurrent(const lifetime& o) const;
                    bool isStack() const;
                };

                RegisterAllocatonStep(Context* ctx, u32 numGP, u32 numFP);
                virtual ~RegisterAllocatonStep();

                virtual bool execute(compiler::CodeHolder* code, Pipeline* pipeline);
            
            protected:
                void getLive(u32 at, utils::Array<lifetime>& live);
                void calcLifetimes();
                bool allocateRegisters(utils::Array<lifetime>& live, u16 k, Pipeline* pipeline);

                u32 m_numGP;
                u32 m_numFP;

                compiler::CodeHolder* m_ch;
                utils::Array<lifetime> m_gpLf;
                utils::Array<lifetime> m_fpLf;
        };
    };
};