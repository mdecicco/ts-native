#include <tsn/vm/Instruction.h>
#include <tsn/vm/VM.h>
#include <tsn/vm/types.h>
#include <tsn/vm/macros.h>
#include <tsn/vm/VMBackend.h>

#include <tsn/common/Context.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/FunctionRegistry.h>

#include <assert.h>

namespace tsn {
    namespace vm {
        using vmi = vm_instruction;
        using vmr = vm_register;

        Instruction::Instruction() : m_code(0), m_imm(0) { }

        Instruction::Instruction(vm_instruction i) : m_code(0), m_imm(0) {
            m_code |= u32(i) << instr_shift;
        }

        Instruction& Instruction::operand(vm_register reg) {
            vmi instr = decode_instr;

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!check_instr_type_1(instr)); // instruction takes a 32 bit integer as the only operand
            assert(!check_flag(op_3_assigned)); // operand 3 already set, no instructions take a 4th operand
            

            // maybe set operand 3
            if (check_flag(op_2_assigned)) {
                // operand 2 already set
                assert(third_operand_is_register(instr)); // instruction does not need third operand or third operand can not be a register
                assert(!((third_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!third_operand_must_be_fpr(instr) && is_fpr(reg)))); // invalid operand

                set_flag(op_3_assigned);
                m_code |= u32(reg) << op_3_shift;
                return *this;
            }
            
            // maybe set operand 2
            if (check_flag(op_1_assigned)) {
                // operand 1 already set
                assert(second_operand_is_register(instr)); // instruction does not need second operand or second operand can not be a register
                assert(!((second_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!second_operand_must_be_fpr(instr) && is_fpr(reg)))); // invalid operand

                set_flag(op_2_assigned);
                m_code |= u32(reg) << op_2_shift;
                return *this;
            }

            // operand 1
            assert(first_operand_is_register(instr)); // instruction does not need an operand or first operand can not be a register
            assert(!(first_operand_must_be_fpr(instr) != is_fpr(reg) && !(first_operand_can_be_fpr(instr) && is_fpr(reg)))); // insnstruction requires operand 1 to be floating point register

            set_flag(op_1_assigned);
            m_code |= u32(reg) << op_1_shift;
            return *this;
        }

        Instruction& Instruction::operand(i64 immediate) {
            return operand(*(u64*)&immediate);
        }

        Instruction& Instruction::operand(u64 immediate) {
            vmi instr = decode_instr;

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!check_flag(op_3_assigned)); // operand 3 already set, no instructions take a 4th operand

            if (check_flag(op_2_assigned)) {
                // operand 2 already set
                assert(third_operand_is_immediate(instr)); // instruction does not take third operand or third operand is not immediate
                assert(!third_operand_must_be_fpi(instr)); // instruction requires that the third operand be floating point
                set_flag(op_3_assigned);
                m_imm = immediate;
                return *this;
            }

            if (check_flag(op_1_assigned)) {
                // operand 1 already set
                assert(second_operand_is_immediate(instr)); // instruction does not take second operand or second operand is not immediate
                set_flag(op_2_assigned);
                m_imm = immediate;
                return *this;
            }

            // operand 1
            assert(first_operand_is_immediate(instr)); // instruction does not take an operand or first operand is not immediate
            set_flag(op_1_assigned);
            m_imm = immediate;
            return *this;
        }

        Instruction& Instruction::operand(f64 immediate) {
            vmi instr = decode_instr;

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!check_flag(op_3_assigned)); // operand 3 already set, no instructions take a 4th operand

            if (check_flag(op_2_assigned)) {
                // operand 2 already set
                assert(third_operand_is_immediate(instr)); // instruction does not take third operand or third operand is not immediate
                assert(third_operand_can_be_float(instr)); // instruction does not take third operand that can be a float
                set_flag(op_3_is_float);
                m_imm = *(u64*)&immediate;
                return *this;
            }

            assert(false); // only third operand can possibly be a float
            return *this;
        }
        
        int _i64toa_s(i64 n, char* buf, size_t bufSz, i32 unk) {
            return snprintf(buf, bufSz, "%lli", n);
        }
        
        int _ui64toa_s(i64 n, char* buf, size_t bufSz, i32 unk) {
            return snprintf(buf, bufSz, "%llu", n);
        }




        constexpr const char* instruction_str[(i32)vm_instruction::instruction_count] = {
            "null",
            "term",

            // memory
            "ld8",
            "ld16",
            "ld32",
            "ld64",
            "st8",
            "st16",
            "st32",
            "st64",
            "mptr",
            "mtfp",
            "mffp",

            // arithmetic
            "add",
            "addi",
            "sub",
            "subi",
            "subir",
            "mul",
            "muli",
            "div",
            "divi",
            "divir",
            "neg",

            // unsigned arithmetic
            "addu",
            "addui",
            "subu",
            "subui",
            "subuir",
            "mulu",
            "mului",
            "divu",
            "divui",
            "divuir",

            // integer / floating point conversion
            "cvt.i.f",
            "cvt.i.d",
            "cvt.i.u",
            "cvt.u.f",
            "cvt.u.d",
            "cvt.u.i",
            "cvt.f.i",
            "cvt.f.u",
            "cvt.f.d",
            "cvt.d.i",
            "cvt.d.u",
            "cvt.d.f",

            // floating point arithmetic
            "fadd",
            "faddi",
            "fsub",
            "fsubi",
            "fsubir",
            "fmul",
            "fmuli",
            "fdiv",
            "fdivi",
            "fdivir",
            "negf",
            "dadd",
            "daddi",
            "dsub",
            "dsubi",
            "dsubir",
            "dmul",
            "dmuli",
            "ddiv",
            "ddivi",
            "ddivir",
            "negd",

            // comparison
            "lt",
            "lti",
            "lte",
            "ltei",
            "gt",
            "gti",
            "gte",
            "gtei",
            "cmp",
            "cmpi",
            "ncmp",
            "ncmpi",
            
            // floating point comparison
            "flt",
            "flti",
            "flte",
            "fltei",
            "fgt",
            "fgti",
            "fgte",
            "fgtei",
            "fcmp",
            "fcmpi",
            "fncmp",
            "fncmpi",
            "dlt",
            "dlti",
            "dlte",
            "dltei",
            "dgt",
            "dgti",
            "dgte",
            "dgtei",
            "dcmp",
            "dcmpi",
            "dncmp",
            "dncmpi",

            // boolean
            "and",
            "andi",
            "or",
            "ori",

            // bitwise
            "band",
            "bandi",
            "bor",
            "bori",
            "xor",
            "xori",
            "sl",
            "sli",
            "slir",
            "sr",
            "sri",
            "srir",

            // control flow
            "beqz",
            "bneqz",
            "bgtz",
            "bgtez",
            "bltz",
            "bltez",
            "jmp",
            "jmpr",
            "jal",
            "jalr"
        };

        constexpr const char* register_str[(i32)vm_register::register_count] = {
            "zero",

            // stores results
            "v0",
            "v1",
            "v2",
            "v3",
            "vf0",
            "vf1",
            "vf2",
            "vf3",

            // function arguments
            "a0",
            "a1",
            "a2",
            "a3",
            "a4",
            "a5",
            "a6",
            "a7",
            "fa0",
            "fa1",
            "fa2",
            "fa3",
            "fa4",
            "fa5",
            "fa6",
            "fa7",

            // temporary storage
            "s0",
            "s1",
            "s2",
            "s3",
            "s4",
            "s5",
            "s6",
            "s7",
            "s8",
            "s9",
            "s10",
            "s11",
            "s12",
            "s13",
            "s14",
            "s15",

            // floating point
            "f0",
            "f1",
            "f2",
            "f3",
            "f4",
            "f5",
            "f6",
            "f7",
            "f8",
            "f9",
            "f10",
            "f11",
            "f12",
            "f13",
            "f14",
            "f15",

            // execution context
            "ip",
            "ra",
            "sp"
        };

        utils::String Instruction::toString(Context* ctx) const {
            Backend* be = (Backend*)ctx->getBackend();
            VM* vm = be->getVM();
            State* state = &vm->state;

            std::string out;
            vmi i = instr();
            if (i >= vmi::instruction_count) return "Invalid Instruction";
            
            if (check_instr_type_0(i)) return instruction_str[(u8)i];

            auto reg_str = [vm](vmr r, i64 offset = 0, bool is_mem = false) {
                if (!vm->isExecuting()) return "$" + std::string(register_str[(u64)r]);
                else {
                    std::string reg_val = "";

                    if (is_fpr(r)) {
                        reg_val = utils::String::Format("<%f>", *(f32*)&vm->state.registers[(u64)r]);
                    }
                    else {
                        i64 val = *(i64*)&vm->state.registers[(u64)r];
                        if (abs(val) > 1000000) reg_val = utils::String::Format("<0x%llX>", (u64)val);
                        else reg_val = utils::String::Format("<%lld>", val);
                    }
                    return "$" + std::string(register_str[(u64)r]) + reg_val;
                }

                return std::string();
            };

            out += instruction_str[(u8)i];
            while (out.length() < 8) out += ' ';

            if (i == vmi::jal && ctx) {
                ffi::Function* fn = ctx->getFunctions()->getFunction((u32)imm_u());
                if (fn) {
                    out += fn->getFullyQualifiedName();
                    return out;
                }
            }

            if (check_instr_type_1(i)) {
                u64 o1 = imm_u();
                out += utils::String::Format("0x%llX", o1);
            } else if (check_instr_type_2(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
            } else if (check_instr_type_3(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
                out += ", ";

                u64 o2 = imm_u();
                out += utils::String::Format("0x%llX", o2);
            } else if (check_instr_type_4(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
                out += ", ";

                vmr o2 = op_2r();
                if (o2 >= vmr::register_count) return "Invalid Instruction";
                out += reg_str(o2);
            } else if (check_instr_type_5(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
                out += ", ";

                vmr o2 = op_2r();
                if (o2 >= vmr::register_count) return "Invalid Instruction";

                u64 o3 = imm_u();
                char offset[32] = { 0 };
                _ui64toa_s(o3, offset, 32, 10);
                out += offset;
                out += '(' + reg_str(o2, o3, true) + ')';
            } else if (check_instr_type_6(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
                out += ", ";

                vmr o2 = op_2r();
                if (o2 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o2);
                out += ", ";

                if (immIsFloat()) {
                    f64 o3 = imm_f();
                    char imm[32] = { 0 };
                    snprintf(imm, 32, "%lf", o3);
                    out += imm;
                } else {
                    i64 o3 = imm_i();
                    char imm[32] = { 0 };
                    _i64toa_s(o3, imm, 32, 10);
                    out += imm;
                }

            } else if (check_instr_type_7(i)) {
                vmr o1 = op_1r();
                if (o1 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o1);
                out += ", ";

                vmr o2 = op_2r();
                if (o2 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o2);
                out += ", ";

                vmr o3 = op_3r();
                if (o3 >= vmr::register_count) return "Invalid Instruction";

                out += reg_str(o3);
            }

            return out;
        }
    };
};