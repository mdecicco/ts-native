#pragma once
#include <common/types.h>
#include <vector>

namespace gjs {
    struct compilation_output;

    /*
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
    class register_allocator {
        public:
            struct reg_lifetime {
                u32 reg_id;
                u32 new_id;
                u32 stack_loc;
                u32 begin;
                u32 end;
                bool is_fp;

                bool is_concurrent(const reg_lifetime& o) const;
                inline bool is_stack() const { return stack_loc != u32(-1); }
            };

            register_allocator(compilation_output& in, u16 gpN, u16 fpN);
            ~register_allocator();

            void process();

            std::vector<reg_lifetime> get_live(u64 at);

        protected:
            void process_func(u16 fidx);
            void calc_reg_lifetimes(u64 from, u64 to);
            void reassign_registers(std::vector<reg_lifetime>& regs, u16 k, u64 from, u64 to, u16 fidx);

            u16 m_gpc;
            u16 m_fpc;
            compilation_output& m_in;
            std::vector<reg_lifetime> m_gpLf;
            std::vector<reg_lifetime> m_fpLf;
            u16 m_nextStackIdx;
    };
};