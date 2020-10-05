#pragma once
#include <common/types.h>
#include <string>

namespace gjs {
    enum class vm_instruction;
    enum class vm_register;

    /*
     * Anatomy of an encoded instruction
     *
     * type 0  | instruction |-----------------------------------------| flags | <- term, null
     * type 1  | instruction |--------------------------------------imm| flags | <- jal, jmp
     * type 2  | instruction |  operand  |-----------------------------| flags | <- jalr, jmpr, cvt_*
     * type 3  | instruction |  operand  |--------------------------imm| flags | <- b*, mptr
     * type 4  | instruction |  operand  |  operand  |-----------------| flags | <- mtfp, mffp
     * type 5  | instruction |  operand  |  operand  |--------------imm| flags | <- ld*, st*
     * type 6  | instruction |  operand  |  operand  |--------------imm| flags | <- *i, *ir
     * type 7  | instruction |  operand  |  operand  |  operand  |-----| flags | <- the rest
     *         |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0| (bits)
     *         |32             |24             |16             |8              | (bytes)
     */

    static const u32 instr_shift = 25;
    static const u32 op_1_shift = 19;
    static const u32 op_2_shift = 13;
    static const u32 op_3_shift = 7; 
    static const u32 op_mask   = 0b11111111111111111111111111000000;
    static const u32 flag_mask = 0b11111111111111111111111111110000;

    class vm_backend;
    class instruction {
        public:
            enum flags {
                op_1_assigned = 0b0001,
                op_2_assigned = 0b0010,
                op_3_assigned = 0b0100,
                op_3_is_float = 0b1000
            };
            instruction();
            instruction(vm_instruction i);
            ~instruction() { }

            instruction& operand(vm_register reg);
            instruction& operand(i64 immediate);
            instruction& operand(u64 immediate);
            instruction& operand(f64 immediate);

            FORCE_INLINE vm_instruction instr() const { return vm_instruction(m_code >> instr_shift); }
            FORCE_INLINE vm_register    op_1r() const { return vm_register   (((m_code >> op_1_shift ) | op_mask) ^ op_mask); }
            FORCE_INLINE vm_register    op_2r() const { return vm_register   (((m_code >> op_2_shift ) | op_mask) ^ op_mask); }
            FORCE_INLINE vm_register    op_3r() const { return vm_register   (((m_code >> op_3_shift ) | op_mask) ^ op_mask); }
            FORCE_INLINE i64            imm_i() const { return *(i64*)&m_imm; }
            FORCE_INLINE u64            imm_u() const { return m_imm; }
            FORCE_INLINE f64            imm_f() const { return *(f64*)&m_imm; }
            FORCE_INLINE bool       imm_is_flt() const { return ((m_code | flag_mask) ^ flag_mask) & op_3_is_float; }

            std::string to_string(vm_backend* ctx = nullptr) const;
        protected:
            friend class vm_backend;
            u32 m_code;
            u64 m_imm;
    };

    FORCE_INLINE instruction encode(vm_instruction i) { return instruction(i); }

};

