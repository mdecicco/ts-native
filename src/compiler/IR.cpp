#include <tsn/compiler/IR.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/FunctionRegistry.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        constexpr ir_instruction_info opcode_info[] = {
            // opcode name, operand count, { op[0] type, op[1] type, op[2] type }, assigns operand index, has side effects
            { "noop"          , 0, { ot_nil, ot_nil, ot_nil }, 0xFF, 0 },
            { "label"         , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF, 0 },
            { "stack_allocate", 2, { ot_imm, ot_imm, ot_nil }, 0xFF, 0 },
            { "stack_ptr"     , 2, { ot_reg, ot_imm, ot_nil }, 0   , 0 },
            { "stack_free"    , 1, { ot_imm, ot_nil, ot_nil }, 0xFF, 0 },
            { "module_data"   , 3, { ot_reg, ot_imm, ot_imm }, 0   , 0 },
            { "reserve"       , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "resolve"       , 2, { ot_reg, ot_val, ot_nil }, 0xFF, 0 },
            { "load"          , 3, { ot_reg, ot_reg, ot_imm }, 0   , 0 },
            { "store"         , 3, { ot_val, ot_reg, ot_imm }, 0xFF, 0 },
            { "jump"          , 1, { ot_lbl, ot_nil, ot_nil }, 0xFF, 0 },
            { "cvt"           , 3, { ot_reg, ot_val, ot_imm }, 0   , 0 },
            { "param"         , 2, { ot_val, ot_imm, ot_nil }, 0xFF, 0 },
            { "call"          , 1, { ot_fun, ot_nil, ot_nil }, 0xFF, 1 },
            { "ret"           , 0, { ot_nil, ot_nil, ot_nil }, 0xFF, 0 },
            { "branch"        , 3, { ot_reg, ot_lbl, ot_lbl }, 0xFF, 0 },
            { "iadd"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "uadd"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fadd"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dadd"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "isub"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "usub"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fsub"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dsub"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "imul"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "umul"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fmul"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dmul"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "idiv"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "udiv"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fdiv"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ddiv"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "imod"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "umod"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fmod"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dmod"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ilt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ult"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "flt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dlt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ilte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ulte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "flte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dlte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "igt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ugt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fgt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dgt"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "igte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ugte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fgte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dgte"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ieq"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ueq"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "feq"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "deq"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "ineq"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "uneq"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "fneq"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "dneq"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "iinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "uinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "finc"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "dinc"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "idec"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "udec"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "fdec"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "ddec"          , 1, { ot_reg, ot_nil, ot_nil }, 0   , 0 },
            { "ineg"          , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 },
            { "fneg"          , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 },
            { "dneg"          , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 },
            { "not"           , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 },
            { "inv"           , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 },
            { "shl"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "shr"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "land"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "band"          , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "lor"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "bor"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "xor"           , 3, { ot_reg, ot_val, ot_val }, 0   , 0 },
            { "assign"        , 2, { ot_reg, ot_val, ot_nil }, 0   , 0 }
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
                if (i == info.assigns_operand_index) {
                    bool readsValueBeforeAssignment = info.operand_count == 1;
                    if (!readsValueBeforeAssignment && excludeAssignment) continue;
                }
                return true;
            }

            return false;
        }

        Instruction& Instruction::operator =(const Instruction& rhs) {
            op = rhs.op;
            operands[0].reset(rhs.operands[0]);
            operands[1].reset(rhs.operands[1]);
            operands[2].reset(rhs.operands[2]);
            src = rhs.src;
            oCnt = rhs.oCnt;
            
            return *this;
        }

        utils::String Instruction::toString(Context* ctx) const {
            if (op == ir_noop && comment.size() > 0) {
                return "; " + comment;
            }

            const ir_instruction_info& info = opcode_info[op];
            utils::String s = info.name;
            for (u8 o = 0;o < oCnt;o++) {
                if (op == ir_cvt && o == 2 && operands[2].isImm()) {
                    ffi::DataType* tp = ctx->getTypes()->getType(operands[2].getImm<type_id>());
                    if (tp) s += utils::String::Format(" <Type %s>", tp->getFullyQualifiedName().c_str());
                    else s += " <Invalid Type ID>";
                    continue;
                } else if (op == ir_module_data) {
                    if (o == 1 && operands[1].isImm() && operands[2].isImm()) {
                        Module* mod = ctx->getModule(operands[1].getImm<u32>());
                        u32 slot = operands[2].getImm<u32>();

                        if (mod) {
                            auto& info = mod->getDataInfo(slot);
                            s += utils::String::Format(" <Module %s : %s>", mod->getName().c_str(), info.name.c_str());
                        } else s += " <Invalid Module ID>";

                        break;
                    }
                } else if ((op == ir_load || op == ir_store) && o == 1 && operands[2].isValid() && operands[2].isImm()) {
                    s += " " + operands[2].toString(ctx) + "(" + operands[1].toString(ctx) + ")";
                    break;
                }

                if (info.operands[o] == ot_fun && operands[o].isImm()) {
                    ffi::Function* fn = nullptr;
                    if (operands[o].isFunctionID()) {
                        fn = ctx->getFunctions()->getFunction(operands[o].getImm<function_id>());
                    } else {
                        FunctionDef* fd = operands[o].getImm<FunctionDef*>();
                        fn = fd->getOutput();
                    }
                    s += utils::String::Format(" <Function %s>", fn->getDisplayName().c_str());
                } else if (info.operands[o] == ot_lbl) {
                    label_id lid = operands[o].getImm<label_id>();
                    s += utils::String::Format(" LABEL_%d", lid);
                } else s += " " + operands[o].toString(ctx);
            }

            bool commentStarted = false;

            if (op == ir_uadd && operands[1].isReg() && operands[2].isImm() && operands[2].getType()->getInfo().is_integral && operands[1].getName().size() > 0) {
                // Is likely a property offset
                u32 offset = operands[2].getImm<u32>();
                auto prop = operands[1].getType()->getProperties().find([offset](const auto& prop) {
                    return prop.offset == offset;
                });

                if (prop) {
                    // Yup
                    s += " ; " + operands[1].getName() + "." + prop->name;
                }
            } else if ((op == ir_load || op == ir_store) && operands[1].getName().size() > 0) {
                u32 offset = 0;
                if (operands[2].isValid() && operands[2].isImm()) offset = operands[2].getImm<u32>();
                // loading/storing in first property
                auto prop = operands[1].getType()->getProperties().find([offset](const auto& prop) {
                    return prop.offset == offset;
                });

                if (prop) {
                    s += " ; " + operands[1].getName() + "." + prop->name;
                }
            } else if (op == ir_param) {
                arg_type at = arg_type(operands[1].getImm<u8>());
                switch (at) {
                    case arg_type::context_ptr: { s += " ; context_ptr"; commentStarted = true; break; }
                    case arg_type::pointer: { s += " ; pointer"; commentStarted = true; break; }
                    case arg_type::value: { s += " ; value"; commentStarted = true; break; }
                    default: break;
                }
            }

            if (comment.size() > 0) {
                if (!commentStarted) s += " ; ";
                else s += ", ";
                s += comment;
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

        InstructionRef& InstructionRef::comment(const utils::String& comment) {
            Instruction& i = m_owner->m_instructions[m_index];
            i.comment = comment;
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