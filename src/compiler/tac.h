#pragma once
#include <vm/instruction.h>
#include <compiler/variable.h>

namespace gjs {
    namespace compile {
        enum class operation {
            null,
            load,
            store,
            stack_alloc,
            stack_free,
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

            // if op[0] is true: continue else jump to op[1]
            branch,
            jump
        };

        struct tac_instruction {
            public:
                tac_instruction(operation op, const source_ref& src);
                ~tac_instruction();
                
                tac_instruction& operand(const var& v);
                tac_instruction& func(vm_function* f);
                std::string to_string() const;

            protected:
                operation op;
                var operands[3];
                vm_function* callee;
                source_ref src;
                u8 op_idx;
        };
    };
};