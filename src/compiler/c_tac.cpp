#include <compiler/tac.h>
#include <common/script_function.h>
#include <common/script_type.h>

namespace gjs {
    namespace compile {
        static const char* op_str[] = {
            "null",
            "load",
            "store",
            "stack_alloc",
            "stack_free",
            "spill",
            "add",
            "sub",
            "mul",
            "div",
            "mod",
            "uadd",
            "usub",
            "umul",
            "udiv",
            "umod",
            "fadd",
            "fsub",
            "fmul",
            "fdiv",
            "fmod",
            "dadd",
            "dsub",
            "dmul",
            "ddiv",
            "dmod",
            "sl",
            "sr",
            "land",
            "lor",
            "band",
            "bor",
            "bxor",
            "ilt",
            "igt",
            "ilte",
            "igte",
            "incmp",
            "icmp",
            "ult",
            "ugt",
            "ulte",
            "ugte",
            "uncmp",
            "ucmp",
            "flt",
            "fgt",
            "flte",
            "fgte",
            "fncmp",
            "fcmp",
            "dlt",
            "dgt",
            "dlte",
            "dgte",
            "dncmp",
            "dcmp",
            "eq",
            "neg",
            "call",
            "param",
            "ret",
            "branch",
            "jump"
        };

        tac_instruction::tac_instruction() : op(operation::null), op_idx(0), callee(nullptr) {
        }

        tac_instruction::tac_instruction(const tac_instruction& rhs) {
            op = rhs.op;
            operands[0] = rhs.operands[0];
            operands[1] = rhs.operands[1];
            operands[2] = rhs.operands[2];
            callee = rhs.callee;
            src = rhs.src;
            op_idx = rhs.op_idx;
        }

        tac_instruction::tac_instruction(operation _op, const source_ref& _src) : op(_op), src(_src), op_idx(0), callee(nullptr) {
        }

        tac_instruction::~tac_instruction() {
        }

        tac_instruction& tac_instruction::operand(const var& v) {
            if (op_idx == 3) return *this;
            operands[op_idx++] = v;
            return *this;
        }
        
        tac_instruction& tac_instruction::func(script_function* f) {
            callee = f;
            return *this;
        }

        std::string tac_instruction::to_string() const {
            std::string out = op_str[(u8)op];
            if (callee) {
                if (callee->signature.return_type->size > 0) return out + " " + callee->name + " -> " + operands[0].to_string();
                else return out + " " + callee->name;
            }
            for (u8 i = 0;i < op_idx;i++) out += " " + operands[i].to_string();
            return out;
        }
    };
};