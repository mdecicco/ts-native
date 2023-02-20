#pragma once
#include <tsn/common/types.h>

#include <utils/String.h>

namespace tsn {
    namespace vm {
        enum class vm_instruction;
        enum class vm_register;
        class VM;

        /*
        * Anatomy of an encoded instruction
        *
        * type 0  |  instruction  |---------------------------------------| flags |
        * type 1  |  instruction  |---------------------------------------| flags |
        * type 2  |  instruction  |  operand  |---------------------------| flags |
        * type 3  |  instruction  |  operand  |---------------------------| flags |
        * type 4  |  instruction  |  operand  |  operand  |---------------| flags |
        * type 5  |  instruction  |  operand  |  operand  |---------------| flags |
        * type 6  |  instruction  |  operand  |  operand  |---------------| flags |
        * type 7  |  instruction  |  operand  |  operand  |  operand  |---| flags |
        *         |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0| (bits)
        *         |32             |24             |16             |8              | (bytes)
        */

        constexpr tsn::u32 instr_shift = 24;
        constexpr tsn::u32 op_1_shift  = 18;
        constexpr tsn::u32 op_2_shift  = 12;
        constexpr tsn::u32 op_3_shift  = 6; 
        constexpr tsn::u32 op_mask     = 0b11111111111111111111111111000000;
        constexpr tsn::u32 flag_mask   = 0b11111111111111111111111111110000;

        class Instruction {
            public:
                enum flags {
                    op_1_assigned = 0b0001,
                    op_2_assigned = 0b0010,
                    op_3_assigned = 0b0100,
                    op_3_is_float = 0b1000
                };
                Instruction();
                Instruction(vm_instruction i);
                ~Instruction() { }

                Instruction& operand(vm_register reg);
                Instruction& operand(i64 immediate);
                Instruction& operand(u64 immediate);
                Instruction& operand(f64 immediate);

                vm_instruction instr() const { return vm_instruction(m_code >> instr_shift); }
                vm_register    op_1r() const { return vm_register   (((m_code >> op_1_shift ) | op_mask) ^ op_mask); }
                vm_register    op_2r() const { return vm_register   (((m_code >> op_2_shift ) | op_mask) ^ op_mask); }
                vm_register    op_3r() const { return vm_register   (((m_code >> op_3_shift ) | op_mask) ^ op_mask); }
                i64            imm_i() const { return *(i64*)&m_imm; }
                u64            imm_u() const { return m_imm; }
                f64            imm_f() const { return *(f64*)&m_imm; }
                bool      immIsFloat() const { return ((m_code | flag_mask) ^ flag_mask) & op_3_is_float; }

                utils::String toString(VM* vm) const;
            protected:
                u32 m_code;
                u64 m_imm;
        };

        #define encode(instr) Instruction(instr)
    };
};