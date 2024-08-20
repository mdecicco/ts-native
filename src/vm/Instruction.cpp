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

        Instruction::Instruction() : m_code({ 0 }), m_imm(0) { }

        Instruction::Instruction(vm_instruction i) : m_code({ 0 }), m_imm(0) {
            m_code.instr = (unsigned int)i;
        }

        Instruction& Instruction::operand(vm_register reg) {
            vmi instr = vmi(m_code.instr);

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!check_instr_type_1(instr)); // instruction takes a 32 bit integer as the only operand
            assert(!m_code.op3_assigned); // operand 3 already set, no instructions take a 4th operand
            

            // maybe set operand 3
            if (m_code.op2_assigned) {
                // operand 2 already set
                assert(third_operand_is_register(instr)); // instruction does not need third operand or third operand can not be a register
                assert(!((third_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!third_operand_must_be_fpr(instr) && is_fpr(reg)))); // invalid operand

                m_code.op3_assigned = 1;
                m_code.op3 = u32(reg);
                return *this;
            }
            
            // maybe set operand 2
            if (m_code.op1_assigned) {
                // operand 1 already set
                assert(second_operand_is_register(instr)); // instruction does not need second operand or second operand can not be a register
                assert(!((second_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!second_operand_must_be_fpr(instr) && is_fpr(reg)))); // invalid operand

                m_code.op2_assigned = 1;
                m_code.op2 = u32(reg);
                return *this;
            }

            // operand 1
            assert(first_operand_is_register(instr)); // instruction does not need an operand or first operand can not be a register
            assert(!(first_operand_must_be_fpr(instr) != is_fpr(reg) && !(first_operand_can_be_fpr(instr) && is_fpr(reg)))); // insnstruction requires operand 1 to be floating point register

            m_code.op1_assigned = 1;
            m_code.op1 = u32(reg);
            return *this;
        }

        Instruction& Instruction::operand(i64 immediate) {
            return operand(*(u64*)&immediate);
        }

        Instruction& Instruction::operand(u64 immediate) {
            vmi instr = vmi(m_code.instr);

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!m_code.op3_assigned); // operand 3 already set, no instructions take a 4th operand

            if (m_code.op2_assigned) {
                // operand 2 already set
                assert(third_operand_is_immediate(instr)); // instruction does not take third operand or third operand is not immediate
                assert(!third_operand_must_be_fpi(instr)); // instruction requires that the third operand be floating point
                m_code.op3_assigned = 1;
                m_imm = immediate;
                return *this;
            }

            if (m_code.op1_assigned) {
                // operand 1 already set
                assert(second_operand_is_immediate(instr)); // instruction does not take second operand or second operand is not immediate
                assert(!second_operand_must_be_fpi(instr));  // instruction requires that the second operand be floating point
                m_code.op2_assigned = 1;
                m_imm = immediate;
                return *this;
            }

            // operand 1
            assert(first_operand_is_immediate(instr)); // instruction does not take an operand or first operand is not immediate
            m_code.op1_assigned = 1;
            m_imm = immediate;
            return *this;
        }

        Instruction& Instruction::operand(f64 immediate) {
            vmi instr = vmi(m_code.instr);

            assert(!check_instr_type_0(instr)); // instruction takes no operands
            assert(!m_code.op3_assigned); // operand 3 already set, no instructions take a 4th operand

            if (m_code.op2_assigned) {
                // operand 2 already set
                assert(third_operand_is_immediate(instr)); // instruction does not take third operand or third operand is not immediate
                assert(third_operand_can_be_float(instr)); // instruction does not take third operand that can be a float
                m_code.imm_is_float = 1;
                m_imm = *(u64*)&immediate;
                return *this;
            }

            if (m_code.op1_assigned) {
                assert(second_operand_is_immediate(instr)); // instruction does not take second operand or second operand is not immediate
                assert(second_operand_must_be_fpi(instr)); // instruction does not take second operand that can be a float
                m_code.imm_is_float = 1;
                m_imm = *(u64*)&immediate;
                return *this;
            }

            assert(false); // operand cannot be a floating point immediate
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

            // vector operations
            "v2fset",
            "v2fsets",
            "v2fsetsi",
            "v2dset",
            "v2dsets",
            "v2dsetsi",
            "v2fadd",
            "v2fadds",
            "v2faddsi",
            "v2dadd",
            "v2dadds",
            "v2daddsi",
            "v2fsub",
            "v2fsubs",
            "v2fsubsi",
            "v2dsub",
            "v2dsubs",
            "v2dsubsi",
            "v2fmul",
            "v2fmuls",
            "v2fmulsi",
            "v2dmul",
            "v2dmuls",
            "v2dmulsi",
            "v2fdiv",
            "v2fdivs",
            "v2fdivsi",
            "v2ddiv",
            "v2ddivs",
            "v2ddivsi",
            "v2fmod",
            "v2fmods",
            "v2fmodsi",
            "v2dmod",
            "v2dmods",
            "v2dmodsi",
            "v2fneg",
            "v2dneg",
            "v2fdot",
            "v2ddot",
            "v2fmag",
            "v2dmag",
            "v2fmagsq",
            "v2dmagsq",
            "v2fnorm",
            "v2dnorm",

            "v3fset",
            "v3fsets",
            "v3fsetsi",
            "v3dset",
            "v3dsets",
            "v3dsetsi",
            "v3fadd",
            "v3fadds",
            "v3faddsi",
            "v3dadd",
            "v3dadds",
            "v3daddsi",
            "v3fsub",
            "v3fsubs",
            "v3fsubsi",
            "v3dsub",
            "v3dsubs",
            "v3dsubsi",
            "v3fmul",
            "v3fmuls",
            "v3fmulsi",
            "v3dmul",
            "v3dmuls",
            "v3dmulsi",
            "v3fdiv",
            "v3fdivs",
            "v3fdivsi",
            "v3ddiv",
            "v3ddivs",
            "v3ddivsi",
            "v3fmod",
            "v3fmods",
            "v3fmodsi",
            "v3dmod",
            "v3dmods",
            "v3dmodsi",
            "v3fneg",
            "v3dneg",
            "v3fdot",
            "v3ddot",
            "v3fmag",
            "v3dmag",
            "v3fmagsq",
            "v3dmagsq",
            "v3fnorm",
            "v3dnorm",
            "v3fcross",
            "v3dcross",
            
            "v4fset",
            "v4fsets",
            "v4fsetsi",
            "v4dset",
            "v4dsets",
            "v4dsetsi",
            "v4fadd",
            "v4fadds",
            "v4faddsi",
            "v4dadd",
            "v4dadds",
            "v4daddsi",
            "v4fsub",
            "v4fsubs",
            "v4fsubsi",
            "v4dsub",
            "v4dsubs",
            "v4dsubsi",
            "v4fmul",
            "v4fmuls",
            "v4fmulsi",
            "v4dmul",
            "v4dmuls",
            "v4dmulsi",
            "v4fdiv",
            "v4fdivs",
            "v4fdivsi",
            "v4ddiv",
            "v4ddivs",
            "v4ddivsi",
            "v4fmod",
            "v4fmods",
            "v4fmodsi",
            "v4dmod",
            "v4dmods",
            "v4dmodsi",
            "v4fneg",
            "v4dneg",
            "v4fdot",
            "v4ddot",
            "v4fmag",
            "v4dmag",
            "v4fmagsq",
            "v4dmagsq",
            "v4fnorm",
            "v4dnorm",
            "v4fcross",
            "v4dcross",

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
            "a8",
            "a9",
            "a10",
            "a11",
            "a12",
            "a13",
            "a14",
            "a15",
            "fa0",
            "fa1",
            "fa2",
            "fa3",
            "fa4",
            "fa5",
            "fa6",
            "fa7",
            "fa8",
            "fa9",
            "fa10",
            "fa11",
            "fa12",
            "fa13",
            "fa14",
            "fa15",

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
            
            if (check_instr_type_0(i)) return instruction_str[(u16)i];

            auto reg_str = [vm](vmr r, i64 offset = 0, bool is_mem = false) {
                if (!vm->isExecuting()) return "$" + std::string(register_str[(u64)r]);
                else {
                    std::string reg_val = "";

                    if (is_fpr(r)) {
                        // sloppy check to see if it's probably a f64 or probably a f32
                        u64 val = vm->state.registers[(u64)r];

                        if (val > u64(0xFFFFFFFF)) {
                            reg_val = utils::String::Format("<%lf>", *(f64*)&val);
                        } else {
                            reg_val = utils::String::Format("<%f>", *(f32*)&val);
                        }
                    } else {
                        i64 val = *(i64*)&vm->state.registers[(u64)r];
                        if (abs(val) > 1000000) reg_val = utils::String::Format("<0x%llX>", (u64)val);
                        else reg_val = utils::String::Format("<%lld>", val);
                    }
                    return "$" + std::string(register_str[(u64)r]) + reg_val;
                }

                return std::string();
            };

            out += instruction_str[(u16)i];
            while (out.length() < 9) out += ' ';

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