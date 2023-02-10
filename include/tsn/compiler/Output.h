#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/utils/SourceLocation.h>

#include <utils/Array.hpp>

namespace tsn {
    class Module;
    class SourceMap;

    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        class OutputBuilder;
        enum ir_instruction;
        enum operand_type : u8;

        namespace output {
            struct operand {
                bool is_imm;
                ffi::DataType* data_type;

                union {
                    vreg_id reg_id;
                    alloc_id alloc_id;
                    function_id func_id;
                    label_id lab_id;
                    u64 imm_u;
                    i64 imm_i;
                    f64 imm_f;
                } value;
            };
            
            struct instruction {
                ir_instruction op;
                operand operands[3];
            };

            struct function {
                ffi::Function* func;
                u32 icount;
                instruction* code;
                SourceMap* map;
            };
        };

        class Output : public IPersistable {
            public:
                Output();
                Output(OutputBuilder* in);
                ~Output();

                Module* getModule();

                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);

            protected:
                Module* m_mod;
                utils::Array<output::function> m_funcs;
        };
    };
};