#include <gs/interfaces/ICodeHolder.h>
#include <gs/compiler/IR.h>

#include <utils/Array.hpp>

namespace gs {
    namespace compiler {
        ICodeHolder::ICodeHolder() {
            m_nextLabelId = 1;
        }

        ICodeHolder::~ICodeHolder() {

        }

        InstructionRef ICodeHolder::add(ir_instruction i, const SourceLocation& src) {
            m_instructions.push(Instruction(i, src));
            return InstructionRef(this, m_instructions.size() - 1);
        }

        label_id ICodeHolder::label(const SourceLocation& src) {
            label_id l = m_nextLabelId++;
            add(ir_label, src).label(l);
            return l;
        }

        void ICodeHolder::remove(const InstructionRef& i) {

        }

        void ICodeHolder::remove(u64 index) {

        }

        // Take care of updating label and vreg IDs
        void ICodeHolder::append(const ICodeHolder& code) {

        }

        // Take care of updating label and vreg IDs
        void ICodeHolder::inlineCall(FunctionDef* f, const utils::Array<Value>& params) {

        }

        const utils::Array<Instruction>& ICodeHolder::getCode() const {
            return m_instructions;
        }
    };
};