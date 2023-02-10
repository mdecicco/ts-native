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
            { "ir_noop"          , 0, { ot_nil, ot_nil, ot_nil }, 0xFF },
            { "ir_label"         , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF },
            { "ir_stack_allocate", 3, { ot_reg, ot_imm, ot_imm }, 0    },
            { "ir_stack_free"    , 1, { ot_imm, ot_nil, ot_nil }, 0xFF },
            { "ir_module_data"   , 3, { ot_reg, ot_imm, ot_imm }, 0    },
            { "ir_reserve"       , 1, { ot_reg, ot_nil, ot_nil }, 0    },
            { "ir_resolve"       , 2, { ot_reg, ot_val, ot_nil }, 0xFF },
            { "ir_load"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_store"         , 2, { ot_val, ot_val, ot_nil }, 0xFF },
            { "ir_jump"          , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF },
            { "ir_cvt"           , 3, { ot_reg, ot_val, ot_imm }, 0    },
            { "ir_param"         , 1, { ot_val, ot_nil, ot_nil }, 0xFF },
            { "ir_call"          , 2, { ot_fun, ot_val, ot_nil }, 1    },
            { "ir_ret"           , 1, { ot_val, ot_nil, ot_nil }, 0xFF },
            { "ir_branch"        , 3, { ot_reg, ot_lbl, ot_lbl }, 0xFF },
            { "ir_iadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_uadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dadd"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_isub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_usub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fsub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dsub"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_imul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_umul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fmul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dmul"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_idiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_udiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fdiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ddiv"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_imod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_umod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fmod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dmod"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ilt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ult"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_flt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dlt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ilte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ulte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_flte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dlte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_igt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ugt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fgt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dgt"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_igte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ugte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fgte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dgte"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ieq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ueq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_feq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_deq"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_ineq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_uneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_fneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_dneq"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_iinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_uinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_finc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_dinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_idec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_udec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_fdec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_ddec"          , 1, { ot_reg, ot_nil, ot_nil }, 0xFF },
            { "ir_ineg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_fneg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_dneg"          , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_not"           , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_inv"           , 2, { ot_reg, ot_val, ot_nil }, 0    },
            { "ir_shl"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_shr"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_land"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_band"          , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_lor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_bor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_xor"           , 3, { ot_reg, ot_val, ot_val }, 0    },
            { "ir_assign"        , 2, { ot_reg, ot_val, ot_nil }, 0    }
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