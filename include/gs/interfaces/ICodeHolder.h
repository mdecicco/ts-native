#pragma once
#include <gs/common/types.h>
#include <gs/compiler/Value.h>
#include <gs/compiler/types.h>

#include <utils/Array.h>
#include <utils/String.h>

namespace gs {
    namespace ffi {
        class DataType;
    };

    namespace compiler {
        class InstructionRef;
        class FunctionDef;
        enum ir_instruction;
        class Instruction;

        class ICodeHolder {
            public:
                ICodeHolder();
                ~ICodeHolder();

                InstructionRef add(ir_instruction i, const SourceLocation& src);
                label_id label(const SourceLocation& src);
                void remove(const InstructionRef& i);
                void remove(u64 index);

                // Takes care of updating label and vreg IDs
                void append(const ICodeHolder& code);

                // Takes care of updating label and vreg IDs
                void inlineCall(FunctionDef* f, const utils::Array<Value>& params);

                const utils::Array<Instruction>& getCode() const;
            
            protected:
                friend class InstructionRef;

                label_id m_nextLabelId;
                utils::Array<Instruction> m_instructions;
        };
    };
};