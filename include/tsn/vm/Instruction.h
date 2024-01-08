#pragma once
#include <tsn/common/types.h>

#include <utils/String.h>

namespace tsn {
    class Context;

    namespace vm {
        enum class vm_instruction;
        enum class vm_register;

        /*
        * Anatomy of an encoded instruction
        *
        * type 0  |   instruction   |-----------------------------------------| flags |
        * type 1  |   instruction   |-----------------------------------------| flags |
        * type 2  |   instruction   |   operand   |---------------------------| flags |
        * type 3  |   instruction   |   operand   |---------------------------| flags |
        * type 4  |   instruction   |   operand   |   operand   |-------------| flags |
        * type 5  |   instruction   |   operand   |   operand   |-------------| flags |
        * type 6  |   instruction   |   operand   |   operand   |-------------| flags |
        * type 7  |   instruction   |   operand   |   operand   |   operand   | flags |
        *         |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0| (bits)
        *         |32             |24             |16             |8              | (bytes)
        */

        struct instr_code {
            unsigned int instr        : 9;
            unsigned int op1          : 7;
            unsigned int op2          : 7;
            unsigned int op3          : 7;
            unsigned int op1_assigned : 1;
            unsigned int op2_assigned : 1;
            unsigned int op3_assigned : 1;
            unsigned int imm_is_float : 1;
        };
        static_assert(sizeof(instr_code) == 8, "sizeof(instr_code) should be 8 bytes");

        class Instruction {
            public:
                Instruction();
                Instruction(vm_instruction i);
                ~Instruction() { }

                Instruction& operand(vm_register reg);
                Instruction& operand(i64 immediate);
                Instruction& operand(u64 immediate);
                Instruction& operand(f64 immediate);

                vm_instruction instr() const { return vm_instruction(m_code.instr); }
                vm_register    op_1r() const { return vm_register   (m_code.op1); }
                vm_register    op_2r() const { return vm_register   (m_code.op2); }
                vm_register    op_3r() const { return vm_register   (m_code.op3); }
                i64            imm_i() const { return *(i64*)&m_imm; }
                u64            imm_u() const { return m_imm; }
                f64            imm_f() const { return *(f64*)&m_imm; }
                bool      immIsFloat() const { return m_code.imm_is_float == 1; }

                utils::String toString(Context* ctx) const;
            protected:
                instr_code m_code;
                u64 m_imm;
        };

        #define encode(instr) Instruction(instr)
    };
};