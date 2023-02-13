#include <tsn/compiler/IR.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/common/Function.h>
#include <tsn/common/FunctionRegistry.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        static const ir_instruction_info opcode_info[] = {
            // opcode name, operand count, { op[0] type, op[1] type, op[2] type }, assigns operand index
            { "noop"          , 0, { ot_nil, ot_nil, ot_nil }, 0xFF },
            { "label"         , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF },
            { "stack_allocate", 3, { ot_reg, ot_imm, ot_imm }, 0    },
            { "stack_free"    , 1, { ot_imm, ot_nil, ot_nil }, 0xFF },
            { "module_data"   , 3, { ot_reg, ot_imm, ot_imm }, 0    },
            { "reserve"       , 1, { ot_reg, ot_nil, ot_nil }, 0    },
            { "resolve"       , 2, { ot_reg, ot_val, ot_nil }, 0xFF },
            { "load"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "store"         , 2, { ot_val, ot_val, ot_nil }, 0xFF },
            { "jump"          , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF },
            { "cvt"           , 3, { ot_reg, ot_val, ot_imm }, 0    },
            { "param"         , 1, { ot_val, ot_nil, ot_nil }, 0xFF },
            { "call"          , 2, { ot_fun, ot_val, ot_nil }, 1    },
            { "ret"           , 1, { ot_val, ot_nil, ot_nil }, 0xFF },
            { "branch"        , 3, { ot_reg, ot_lbl, ot_lbl }, 0xFF },
            { "iadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "uadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "isub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "usub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fsub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dsub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "imul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "umul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fmul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dmul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "idiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "udiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fdiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ddiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "imod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "umod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fmod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dmod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ilt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ult"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "flt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dlt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ilte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ulte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "flte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dlte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "igt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ugt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fgt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dgt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "igte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ugte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fgte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dgte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ieq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ueq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "feq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "deq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ineq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "uneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "fneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "dneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "iinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "uinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "finc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "dinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "idec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "udec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "fdec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ddec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ineg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "fneg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "dneg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "not"           , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "inv"           , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "shl"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "shr"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "land"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "band"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "lor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "bor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "xor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "assign"        , 2, { ot_reg, ot_val, ot_nil }, 0    }
        };

        const ir_instruction_info& instruction_info(ir_instruction op) {
            return opcode_info[op];
        }

        //
        // Instruction
        //
        Instruction::Instruction() {
            op = ir_noop;
            oCnt = 0;
        }

        Instruction::Instruction(ir_instruction i, const SourceLocation& _src) : src(_src) {
            op = i;
            oCnt = 0;
        }

        const Value* Instruction::assigns() const {
            const ir_instruction_info& info = opcode_info[op];
            if (info.assigns_operand_index == 0xFF) return nullptr;
            return &operands[info.assigns_operand_index];
        }

        bool Instruction::involves(vreg_id reg, bool excludeAssignment) const {
            const ir_instruction_info& info = opcode_info[op];
            
            for (u8 i = 0;i < oCnt;i++) {
                if (!operands[i].isReg() || operands[i].getRegId() != reg) continue;
                if (i == info.assigns_operand_index && excludeAssignment) continue;
                return true;
            }

            return false;
        }

        utils::String Instruction::toString(Context* ctx) const {
            const ir_instruction_info& info = opcode_info[op];
            utils::String s = info.name;
            for (u8 o = 0;o < oCnt;o++) {
                if (info.operands[o] == ot_fun && operands[o].isImm()) {
                    FunctionDef* fd = operands[o].getImm<FunctionDef*>();
                    ffi::Function* fn = fd->getOutput();
                    s += utils::String::Format(" <Function %s>", fn->getFullyQualifiedName().c_str());
                } else if (info.operands[o] == ot_lbl) {
                    label_id lid = operands[o].getImm<label_id>();
                    s += utils::String::Format(" LABEL_%d", lid);
                } else s += " " + operands[o].toString();
            }

            return s;
        }

        //
        // InstructionRef
        //
        
        InstructionRef::InstructionRef(FunctionDef* owner, u32 index) {
            m_owner = owner;
            m_index = index;
        }

        InstructionRef& InstructionRef::instr(ir_instruction i) {
            m_owner->m_instructions[m_index].op = i;
            return *this;
        }

        InstructionRef& InstructionRef::op(const Value& v) {
            Instruction& i = m_owner->m_instructions[m_index];
            i.operands[i.oCnt++].reset(v);
            return *this;
        }

        InstructionRef& InstructionRef::op(FunctionDef* fn) {
            Instruction& i = m_owner->m_instructions[m_index];
            ffi::Function* f = fn->getOutput();

            i.operands[i.oCnt++].reset(m_owner->imm(f->getId()));

            return *this;
        }

        InstructionRef& InstructionRef::label(label_id l) {
            Instruction& i = m_owner->m_instructions[m_index];
            i.operands[i.oCnt++].reset(m_owner->imm(l));
            return *this;
        }

        const Value* InstructionRef::assigns() const {
            return m_owner->m_instructions[m_index].assigns();
        }

        bool InstructionRef::involves(vreg_id reg, bool excludeAssignment) const {
            return m_owner->m_instructions[m_index].involves(reg, excludeAssignment);
        }

        utils::String InstructionRef::toString(Context* ctx) const {
            return m_owner->m_instructions[m_index].toString(ctx);
        }
    };
};