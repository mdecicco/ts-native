#include <gs/compiler/IR.h>
#include <gs/compiler/FunctionDef.hpp>
#include <gs/common/Function.h>
#include <gs/interfaces/ICodeHolder.h>

#include <utils/Array.hpp>

namespace gs {
    namespace compiler {
        //
        // Instruction
        //

        Instruction::Instruction(ir_instruction i, const SourceLocation& _src) : src(_src) {
            op = i;
            labels[0] = labels[1] = 0;
            oCnt = lCnt = 0;
            fn_operands[0] = fn_operands[1] = fn_operands[2] = nullptr;
        }

        Value* Instruction::assigns() const {
            return nullptr;
        }

        bool Instruction::involves(vreg_id reg, bool excludeAssignment) const {
            return false;
        }


        //
        // InstructionRef
        //
        
        InstructionRef::InstructionRef(ICodeHolder* owner, u32 index) {
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

            if (f) i.operands[i.oCnt++].reset(fn->imm(f->getId()));
            else i.fn_operands[i.oCnt++] = fn;

            return *this;
        }

        InstructionRef& InstructionRef::label(label_id l) {
            Instruction& i = m_owner->m_instructions[m_index];
            i.labels[i.lCnt++] = l;
            return *this;
        }

        Value* InstructionRef::assigns() const {
            Instruction& i = m_owner->m_instructions[m_index];
            return i.assigns();
        }

        bool InstructionRef::involves(vreg_id reg, bool excludeAssignment) const {
            Instruction& i = m_owner->m_instructions[m_index];
            return i.involves(reg, excludeAssignment);
        }

        void InstructionRef::remove() {
            m_owner->remove(m_index);
        }

        utils::String InstructionRef::toString() const {
            return "";
        }
    };
};