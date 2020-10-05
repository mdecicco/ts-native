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
            "module_data",
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
            "cvt",
            "branch",
            "jump"
        };

        bool is_assignment(operation op) {
            static bool s_is_assignment[] = {
                false,   // null
                true,    // load
                false,   // store
                true,    // stack_alloc
                false,   // stack_free
                true,    // module_data
                true,    // iadd
                true,    // isub
                true,    // imul
                true,    // idiv
                true,    // imod
                true,    // uadd
                true,    // usub
                true,    // umul
                true,    // udiv
                true,    // umod
                true,    // fadd
                true,    // fsub
                true,    // fmul
                true,    // fdiv
                true,    // fmod
                true,    // dadd
                true,    // dsub
                true,    // dmul
                true,    // ddiv
                true,    // dmod
                true,    // sl
                true,    // sr
                true,    // land
                true,    // lor
                true,    // band
                true,    // bor
                true,    // bxor
                true,    // ilt
                true,    // igt
                true,    // ilte
                true,    // igte
                true,    // incmp
                true,    // icmp
                true,    // ult
                true,    // ugt
                true,    // ulte
                true,    // ugte
                true,    // uncmp
                true,    // ucmp
                true,    // flt
                true,    // fgt
                true,    // flte
                true,    // fgte
                true,    // fncmp
                true,    // fcmp
                true,    // dlt
                true,    // dgt
                true,    // dlte
                true,    // dgte
                true,    // dncmp
                true,    // dcmp
                true,    // eq
                true,    // neg
                true,    // call
                false,   // param
                false,   // ret
                false,   // cvt
                false,   // branch
                false    // jump
            };

            return s_is_assignment[u32(op)];
        }

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