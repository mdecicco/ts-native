#pragma once
#include <vm/instruction.h>
#include <compiler/variable.h>

namespace gjs {
    namespace compile {
        enum class operation {
            null = 0,

            // load can be in any of the following formats:
            // load dest_var imm_addr
            // load dest_var var_addr
            // load dest_var var_addr imm_offset
            load, // load dest src
            store, // store dest src
            stack_alloc,
            stack_free,
            spill,
            iadd,
            isub,
            imul,
            idiv,
            imod,
            uadd,
            usub,
            umul,
            udiv,
            umod,
            fadd,
            fsub,
            fmul,
            fdiv,
            fmod,
            dadd,
            dsub,
            dmul,
            ddiv,
            dmod,
            sl,
            sr,
            land,
            lor,
            band,
            bor,
            bxor,
            ilt,
            igt,
            ilte,
            igte,
            incmp,
            icmp,
            ult,
            ugt,
            ulte,
            ugte,
            uncmp,
            ucmp,
            flt,
            fgt,
            flte,
            fgte,
            fncmp,
            fcmp,
            dlt,
            dgt,
            dlte,
            dgte,
            dncmp,
            dcmp,
            eq,
            neg,
            call,
            param,
            ret,
            cvt,

            // if op[0] is true: continue else jump to op[1]
            branch,
            jump
        };

        bool is_assignment(operation op);

        struct tac_instruction {
            public:
                tac_instruction();
                tac_instruction(const tac_instruction& rhs);
                tac_instruction(operation op, const source_ref& src);
                ~tac_instruction();
                
                tac_instruction& operand(const var& v);
                tac_instruction& func(script_function* f);
                std::string to_string() const;

                operation op;
                var operands[3];
                script_function* callee;
                source_ref src;

            protected:
                u8 op_idx;
        };
    };
};