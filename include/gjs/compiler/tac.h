#pragma once
#include <gjs/compiler/variable.h>

namespace gjs {
    namespace compile {
        enum class operation {
            null = 0,

            // load can be in any of the following formats:
            // load dest_var imm_addr
            // load dest_var var_addr
            // load dest_var var_addr imm_offset
            load, // load dest src | load dest src imm_offset
            store, // store dest src
            stack_alloc, // stack_alloc dest imm_size
            stack_free, // stack_free obj
            module_data, // module_data imm_module_id imm_offset

            // reserves a virtual register which will be assigned later via resolve
            // reserve acts as the assignment so that the register will be considered live
            // A reserved register is guaranteed to be resolved exactly one time during
            // execution
            reserve,
            resolve,

            iadd,
            isub,
            imul,
            idiv,
            imod,
            uadd,
            usub,
            umul,
            udiv,
            umod,
            fadd,
            fsub,
            fmul,
            fdiv,
            fmod,
            dadd,
            dsub,
            dmul,
            ddiv,
            dmod,
            sl,
            sr,
            land,
            lor,
            band,
            bor,
            bxor,
            ilt,
            igt,
            ilte,
            igte,
            incmp,
            icmp,
            ult,
            ugt,
            ulte,
            ugte,
            uncmp,
            ucmp,
            flt,
            fgt,
            flte,
            fgte,
            fncmp,
            fcmp,
            dlt,
            dgt,
            dlte,
            dgte,
            dncmp,
            dcmp,
            eq,
            neg,
            cvt,
            call,
            param,
            ret,

            label,
            // if op[0] is true: continue, else jump to label[0]
            branch,
            jump,
            term,


            // The following instructions are not required to be used in execution,
            // they just provide extra information about control flow

            // inserted just before the branch instruction for an if statement
            // operands: 
            //   label[0] = end of true case block label
            //   label[1] = end of false case block label, or 0
            //   label[2] = post-branch label
            // meta_if_branch,

            // inserted at the top of a for loop
            // operands:
            //   label[0] = loop branch label
            //   label[1] = loop end label
            // meta_for_loop,

            // inserted at the top of a while loop
            // operands:
            //   label[0] = loop branch label
            //   label[1] = loop end label
            // meta_while_loop,

            // inserted at the top of a do...while loop
            //   note:
            //     do...while loops will always have a jump
            //     instruction immediately following the
            //     branch (jumps to the top, not out)
            // operands:
            //   label[0] = loop branch label
            // meta_do_while_loop
        };

        struct tac_instruction {
            public:
                tac_instruction();
                tac_instruction(const tac_instruction& rhs);
                tac_instruction(operation op, const source_ref& src);
                ~tac_instruction();

                tac_instruction& operand(const var& v);
                tac_instruction& resolve(u8 opIdx, script_function* f);
                tac_instruction& func(script_function* f);
                tac_instruction& func(var f);
                tac_instruction& label(label_id label);
                std::string to_string() const;
                const var* assignsTo() const;
                bool involves(u32 reg_id, bool excludeAssignment = false) const;

                operation op;
                var operands[3];
                script_type* types[3];
                label_id labels[3];
                script_function* callee;
                script_function* resolve_func_ids[3];
                var callee_v;
                source_ref src;

            protected:
                u8 op_idx;
                u16 lb_idx;
        };

        struct context;
        struct tac_wrapper {
            public:
                tac_wrapper();
                tac_wrapper(context* ctx, u64 addr, u16 fidx);

                tac_wrapper& set_operation(operation op);
                tac_wrapper& operand(const var& v);
                tac_wrapper& resolve(u8 opIdx, script_function* f);
                tac_wrapper& func(script_function* f);
                tac_wrapper& func(var f);
                tac_wrapper& label(label_id label);
                std::string to_string() const;

                operator bool();

            protected:
                context* ctx;
                u64 addr;
                u16 fidx;
        };
    
        bool is_assignment(const tac_instruction& i);
    };
};
