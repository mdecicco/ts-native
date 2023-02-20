#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/utils/SourceLocation.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace tsn {
    class Module;
    class ModuleSource;
    class SourceMap;
    struct script_metadata;

    namespace ffi {
        class DataType;
        class Function;
        enum data_type_instance;
        class FunctionRegistry;
        class DataTypeRegistry;
    };

    namespace compiler {
        class OutputBuilder;
        class TemplateContext;
        enum ir_instruction;
        enum operand_type : u8;

        namespace output {
            struct operand {
                struct {
                    unsigned is_reg : 1;
                    unsigned is_stack : 1;
                    unsigned is_func : 1;
                    unsigned is_imm : 1;
                    unsigned _unused : 4;
                } flags;

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
            
            struct proto_function {
                function_id id;
                utils::String name;
                utils::String displayName;
                utils::String fullyQualifiedName;
                access_modifier access;
                type_id signatureTypeId;
                bool isTemplate;
                bool isMethod;
                SourceLocation src;

                // methods only
                u64 baseOffset;

                // templates only
                TemplateContext* tctx;
            };

            struct proto_type_prop {
                utils::String name;
                access_modifier access;
                u64 offset;
                type_id typeId;
                value_flags flags;
                function_id getterId;
                function_id setterId;
            };

            struct proto_type_base {
                type_id typeId;
                u64 offset;
                access_modifier access;
            };

            struct proto_type_arg {
                arg_type argType;
                type_id dataTypeId;
            };

            struct proto_type {
                type_id id;
                ffi::data_type_instance itype;
                utils::String name;
                utils::String fullyQualifiedName;
                type_meta info;
                access_modifier access;
                function_id destructorId;
                utils::Array<proto_type_prop> props;
                utils::Array<proto_type_base> bases;
                utils::Array<function_id> methodIds;
                type_id templateBaseId;
                utils::Array<type_id> templateArgIds;

                // function types only
                function_id returnTypeId;
                utils::Array<proto_type_arg> args;

                // template types only
                TemplateContext* tctx;

                // alias types only
                type_id aliasTypeId;
            };
        };

        class Output : public IPersistable {
            public:
                Output(const script_metadata* meta, ModuleSource* src);
                Output(OutputBuilder* in);
                ~Output();

                Module* getModule();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                bool generateTypesAndFunctions(
                    utils::Array<output::proto_function>& funcs,
                    utils::Array<output::proto_type>& types,
                    ffi::FunctionRegistry* freg,
                    ffi::DataTypeRegistry* treg
                );

                Module* m_mod;
                ModuleSource* m_src;

                const script_metadata* m_meta;
                utils::Array<output::function> m_funcs;
        };
    };
};