#pragma once
// term, null
#define check_instr_type_0(x)             \
    (                                     \
        x == vmi::term                    \
        || x == vmi::null                 \
    )

// jal, jmp
#define check_instr_type_1(x)             \
    (                                     \
        x == vmi::jal                     \
        || x == vmi::jmp                  \
    )

// jalr, jmpr, cvt_*
#define check_instr_type_2(x)             \
    (                                     \
        x == vmi::jalr                    \
        || x == vmi::jmpr                 \
        || x == vmi::cvt_if               \
        || x == vmi::cvt_id               \
        || x == vmi::cvt_iu               \
        || x == vmi::cvt_uf               \
        || x == vmi::cvt_ud               \
        || x == vmi::cvt_ui               \
        || x == vmi::cvt_fi               \
        || x == vmi::cvt_fu               \
        || x == vmi::cvt_fd               \
        || x == vmi::cvt_di               \
        || x == vmi::cvt_du               \
        || x == vmi::cvt_df               \
        || x == vmi::v2fneg               \
        || x == vmi::v2dneg               \
        || x == vmi::v2fnorm              \
        || x == vmi::v2dnorm              \
        || x == vmi::v3fneg               \
        || x == vmi::v3dneg               \
        || x == vmi::v3fnorm              \
        || x == vmi::v3dnorm              \
        || x == vmi::v4fneg               \
        || x == vmi::v4dneg               \
        || x == vmi::v4fnorm              \
        || x == vmi::v4dnorm              \
    )

// b*, mptr
#define check_instr_type_3(x)             \
    (                                     \
        x == vmi::beqz                    \
        || x == vmi::bneqz                \
        || x == vmi::bgtz                 \
        || x == vmi::bgtez                \
        || x == vmi::bltz                 \
        || x == vmi::bltez                \
        || x == vmi::mptr                 \
        || x == vmi::v2fsetsi             \
        || x == vmi::v2dsetsi             \
        || x == vmi::v2faddsi             \
        || x == vmi::v2daddsi             \
        || x == vmi::v2fsubsi             \
        || x == vmi::v2dsubsi             \
        || x == vmi::v2fmulsi             \
        || x == vmi::v2dmulsi             \
        || x == vmi::v2fdivsi             \
        || x == vmi::v2ddivsi             \
        || x == vmi::v2fmodsi             \
        || x == vmi::v2dmodsi             \
        || x == vmi::v3fsetsi             \
        || x == vmi::v3dsetsi             \
        || x == vmi::v3faddsi             \
        || x == vmi::v3daddsi             \
        || x == vmi::v3fsubsi             \
        || x == vmi::v3dsubsi             \
        || x == vmi::v3fmulsi             \
        || x == vmi::v3dmulsi             \
        || x == vmi::v3fdivsi             \
        || x == vmi::v3ddivsi             \
        || x == vmi::v3fmodsi             \
        || x == vmi::v3dmodsi             \
        || x == vmi::v4fsetsi             \
        || x == vmi::v4dsetsi             \
        || x == vmi::v4faddsi             \
        || x == vmi::v4daddsi             \
        || x == vmi::v4fsubsi             \
        || x == vmi::v4dsubsi             \
        || x == vmi::v4fmulsi             \
        || x == vmi::v4dmulsi             \
        || x == vmi::v4fdivsi             \
        || x == vmi::v4ddivsi             \
        || x == vmi::v4fmodsi             \
        || x == vmi::v4dmodsi             \
    )

// mtfp, mffp, neg*
#define check_instr_type_4(x)             \
    (                                     \
        x == vmi::mtfp                    \
        || x == vmi::mffp                 \
        || x == vmi::neg                  \
        || x == vmi::negf                 \
        || x == vmi::negd                 \
        || x == vmi::v2fset               \
        || x == vmi::v2fsets              \
        || x == vmi::v2dset               \
        || x == vmi::v2dsets              \
        || x == vmi::v2fadd               \
        || x == vmi::v2fadds              \
        || x == vmi::v2dadd               \
        || x == vmi::v2dadds              \
        || x == vmi::v2fsub               \
        || x == vmi::v2fsubs              \
        || x == vmi::v2dsub               \
        || x == vmi::v2dsubs              \
        || x == vmi::v2fmul               \
        || x == vmi::v2fmuls              \
        || x == vmi::v2dmul               \
        || x == vmi::v2dmuls              \
        || x == vmi::v2fdiv               \
        || x == vmi::v2fdivs              \
        || x == vmi::v2ddiv               \
        || x == vmi::v2ddivs              \
        || x == vmi::v2fmod               \
        || x == vmi::v2fmods              \
        || x == vmi::v2dmod               \
        || x == vmi::v2dmods              \
        || x == vmi::v2fmag               \
        || x == vmi::v2dmag               \
        || x == vmi::v2fmagsq             \
        || x == vmi::v2dmagsq             \
        || x == vmi::v3fset               \
        || x == vmi::v3fsets              \
        || x == vmi::v3dset               \
        || x == vmi::v3dsets              \
        || x == vmi::v3fadd               \
        || x == vmi::v3fadds              \
        || x == vmi::v3dadd               \
        || x == vmi::v3dadds              \
        || x == vmi::v3fsub               \
        || x == vmi::v3fsubs              \
        || x == vmi::v3dsub               \
        || x == vmi::v3dsubs              \
        || x == vmi::v3fmul               \
        || x == vmi::v3fmuls              \
        || x == vmi::v3dmul               \
        || x == vmi::v3dmuls              \
        || x == vmi::v3fdiv               \
        || x == vmi::v3fdivs              \
        || x == vmi::v3ddiv               \
        || x == vmi::v3ddivs              \
        || x == vmi::v3fmod               \
        || x == vmi::v3fmods              \
        || x == vmi::v3dmod               \
        || x == vmi::v3dmods              \
        || x == vmi::v3fmag               \
        || x == vmi::v3dmag               \
        || x == vmi::v3fmagsq             \
        || x == vmi::v3dmagsq             \
        || x == vmi::v4fset               \
        || x == vmi::v4fsets              \
        || x == vmi::v4dset               \
        || x == vmi::v4dsets              \
        || x == vmi::v4fadd               \
        || x == vmi::v4fadds              \
        || x == vmi::v4dadd               \
        || x == vmi::v4dadds              \
        || x == vmi::v4fsub               \
        || x == vmi::v4fsubs              \
        || x == vmi::v4dsub               \
        || x == vmi::v4dsubs              \
        || x == vmi::v4fmul               \
        || x == vmi::v4fmuls              \
        || x == vmi::v4dmul               \
        || x == vmi::v4dmuls              \
        || x == vmi::v4fdiv               \
        || x == vmi::v4fdivs              \
        || x == vmi::v4ddiv               \
        || x == vmi::v4ddivs              \
        || x == vmi::v4fmod               \
        || x == vmi::v4fmods              \
        || x == vmi::v4dmod               \
        || x == vmi::v4dmods              \
        || x == vmi::v4fmag               \
        || x == vmi::v4dmag               \
        || x == vmi::v4fmagsq             \
        || x == vmi::v4dmagsq             \
    )

// ld*, st*
#define check_instr_type_5(x)             \
    (                                     \
        x == vmi::ld8                     \
        || x == vmi::ld16                 \
        || x == vmi::ld32                 \
        || x == vmi::ld64                 \
        || x == vmi::st8                  \
        || x == vmi::st16                 \
        || x == vmi::st32                 \
        || x == vmi::st64                 \
    )

// *i, *ir
#define check_instr_type_6(x)             \
    (                                     \
        x == vmi::addi                    \
        || x == vmi::subi                 \
        || x == vmi::subir                \
        || x == vmi::muli                 \
        || x == vmi::divi                 \
        || x == vmi::divir                \
        || x == vmi::addui                \
        || x == vmi::subui                \
        || x == vmi::subuir               \
        || x == vmi::mului                \
        || x == vmi::divui                \
        || x == vmi::divuir               \
        || x == vmi::lti                  \
        || x == vmi::ltei                 \
        || x == vmi::gti                  \
        || x == vmi::gtei                 \
        || x == vmi::cmpi                 \
        || x == vmi::ncmpi                \
        || x == vmi::faddi                \
        || x == vmi::fsubi                \
        || x == vmi::fsubir               \
        || x == vmi::fmuli                \
        || x == vmi::fdivi                \
        || x == vmi::fdivir               \
        || x == vmi::flti                 \
        || x == vmi::fltei                \
        || x == vmi::fgti                 \
        || x == vmi::fgtei                \
        || x == vmi::fcmpi                \
        || x == vmi::fncmpi               \
        || x == vmi::daddi                \
        || x == vmi::dsubi                \
        || x == vmi::dsubir               \
        || x == vmi::dmuli                \
        || x == vmi::ddivi                \
        || x == vmi::ddivir               \
        || x == vmi::dlti                 \
        || x == vmi::dltei                \
        || x == vmi::dgti                 \
        || x == vmi::dgtei                \
        || x == vmi::dcmpi                \
        || x == vmi::dncmpi               \
        || x == vmi::bandi                \
        || x == vmi::bori                 \
        || x == vmi::xori                 \
        || x == vmi::sli                  \
        || x == vmi::slir                 \
        || x == vmi::sri                  \
        || x == vmi::srir                 \
        || x == vmi::andi                 \
        || x == vmi::ori                  \
    )

// the rest
#define check_instr_type_7(x)             \
    (                                     \
        x == vmi::add                     \
        || x == vmi::sub                  \
        || x == vmi::mul                  \
        || x == vmi::div                  \
        || x == vmi::addu                 \
        || x == vmi::subu                 \
        || x == vmi::mulu                 \
        || x == vmi::divu                 \
        || x == vmi::lt                   \
        || x == vmi::lte                  \
        || x == vmi::gt                   \
        || x == vmi::gte                  \
        || x == vmi::cmp                  \
        || x == vmi::ncmp                 \
        || x == vmi::fadd                 \
        || x == vmi::fsub                 \
        || x == vmi::fmul                 \
        || x == vmi::fdiv                 \
        || x == vmi::flt                  \
        || x == vmi::flte                 \
        || x == vmi::fgt                  \
        || x == vmi::fgte                 \
        || x == vmi::fcmp                 \
        || x == vmi::fncmp                \
        || x == vmi::dadd                 \
        || x == vmi::dsub                 \
        || x == vmi::dmul                 \
        || x == vmi::ddiv                 \
        || x == vmi::dlt                  \
        || x == vmi::dlte                 \
        || x == vmi::dgt                  \
        || x == vmi::dgte                 \
        || x == vmi::dcmp                 \
        || x == vmi::dncmp                \
        || x == vmi::_and                 \
        || x == vmi::_or                  \
        || x == vmi::band                 \
        || x == vmi::bor                  \
        || x == vmi::_xor                 \
        || x == vmi::sl                   \
        || x == vmi::sr                   \
        || x == vmi::v2fdot               \
        || x == vmi::v2ddot               \
        || x == vmi::v3fdot               \
        || x == vmi::v3ddot               \
        || x == vmi::v3fcross             \
        || x == vmi::v3dcross             \
        || x == vmi::v4fdot               \
        || x == vmi::v4ddot               \
        || x == vmi::v4fcross             \
        || x == vmi::v4dcross             \
    )

#define first_operand_is_register(x)      \
    (                                     \
        !check_instr_type_0(x)            \
        && !check_instr_type_1(x)         \
    )

#define first_operand_is_immediate(x)     \
    check_instr_type_1(x)

#define second_operand_is_register(x)     \
    (                                     \
        !check_instr_type_0(x)            \
        && !check_instr_type_1(x)         \
        && !check_instr_type_2(x)         \
        && !check_instr_type_3(x)         \
    )

#define second_operand_is_immediate(x)    \
    check_instr_type_3(x)

#define third_operand_is_register(x)      \
    check_instr_type_7(x)

#define third_operand_is_immediate(x)     \
    (                                     \
        check_instr_type_5(x)             \
        || check_instr_type_6(x)          \
    )

#define third_operand_can_be_float(x)     \
    check_instr_type_6(x)

#define first_operand_must_be_fpr(x)      \
    (                                     \
        x == vmi::mffp                    \
        || x == vmi::fadd                 \
        || x == vmi::fsub                 \
        || x == vmi::fmul                 \
        || x == vmi::fdiv                 \
        || x == vmi::faddi                \
        || x == vmi::fsubi                \
        || x == vmi::fsubir               \
        || x == vmi::fmuli                \
        || x == vmi::fdivi                \
        || x == vmi::fdivir               \
        || x == vmi::dadd                 \
        || x == vmi::dsub                 \
        || x == vmi::dmul                 \
        || x == vmi::ddiv                 \
        || x == vmi::daddi                \
        || x == vmi::dsubi                \
        || x == vmi::dsubir               \
        || x == vmi::dmuli                \
        || x == vmi::ddivi                \
        || x == vmi::ddivir               \
        || x == vmi::negf                 \
        || x == vmi::negd                 \
        || x == vmi::v2fdot               \
        || x == vmi::v2ddot               \
        || x == vmi::v2fmag               \
        || x == vmi::v2dmag               \
        || x == vmi::v2fmagsq             \
        || x == vmi::v2dmagsq             \
        || x == vmi::v3fdot               \
        || x == vmi::v3ddot               \
        || x == vmi::v3fmag               \
        || x == vmi::v3dmag               \
        || x == vmi::v3fmagsq             \
        || x == vmi::v3dmagsq             \
        || x == vmi::v4fdot               \
        || x == vmi::v4ddot               \
        || x == vmi::v4fmag               \
        || x == vmi::v4dmag               \
        || x == vmi::v4fmagsq             \
        || x == vmi::v4dmagsq             \
    )

// cvt_*, ld*, st*
#define first_operand_can_be_fpr(x)       \
    (                                     \
        x == vmi::ld8                     \
        || x == vmi::ld16                 \
        || x == vmi::ld32                 \
        || x == vmi::ld64                 \
        || x == vmi::st8                  \
        || x == vmi::st16                 \
        || x == vmi::st32                 \
        || x == vmi::st64                 \
        || x == vmi::cvt_if               \
        || x == vmi::cvt_id               \
        || x == vmi::cvt_uf               \
        || x == vmi::cvt_ud               \
        || x == vmi::cvt_fi               \
        || x == vmi::cvt_fu               \
        || x == vmi::cvt_fd               \
        || x == vmi::cvt_di               \
        || x == vmi::cvt_du               \
        || x == vmi::cvt_df               \
    )

#define second_operand_must_be_fpr(x)     \
    (                                     \
        x == vmi::mtfp                    \
        || x == vmi::fadd                 \
        || x == vmi::fsub                 \
        || x == vmi::fmul                 \
        || x == vmi::fdiv                 \
        || x == vmi::faddi                \
        || x == vmi::fsubi                \
        || x == vmi::fsubir               \
        || x == vmi::fmuli                \
        || x == vmi::fdivi                \
        || x == vmi::fdivir               \
        || x == vmi::flti                 \
        || x == vmi::fltei                \
        || x == vmi::fgti                 \
        || x == vmi::fgtei                \
        || x == vmi::fcmpi                \
        || x == vmi::fncmpi               \
        || x == vmi::flt                  \
        || x == vmi::flte                 \
        || x == vmi::fgt                  \
        || x == vmi::fgte                 \
        || x == vmi::fcmp                 \
        || x == vmi::fncmp                \
        || x == vmi::dadd                 \
        || x == vmi::dsub                 \
        || x == vmi::dmul                 \
        || x == vmi::ddiv                 \
        || x == vmi::daddi                \
        || x == vmi::dsubi                \
        || x == vmi::dsubir               \
        || x == vmi::dmuli                \
        || x == vmi::ddivi                \
        || x == vmi::ddivir               \
        || x == vmi::dlti                 \
        || x == vmi::dltei                \
        || x == vmi::dgti                 \
        || x == vmi::dgtei                \
        || x == vmi::dcmpi                \
        || x == vmi::dncmpi               \
        || x == vmi::dlt                  \
        || x == vmi::dlte                 \
        || x == vmi::dgt                  \
        || x == vmi::dgte                 \
        || x == vmi::dcmp                 \
        || x == vmi::dncmp                \
        || x == vmi::negf                 \
        || x == vmi::negd                 \
        || x == vmi::v2fsets              \
        || x == vmi::v2dsets              \
        || x == vmi::v2fadds              \
        || x == vmi::v2dadds              \
        || x == vmi::v2fsubs              \
        || x == vmi::v2dsubs              \
        || x == vmi::v2fmuls              \
        || x == vmi::v2dmuls              \
        || x == vmi::v2fdivs              \
        || x == vmi::v2ddivs              \
        || x == vmi::v2fmods              \
        || x == vmi::v2dmods              \
        || x == vmi::v3fsets              \
        || x == vmi::v3dsets              \
        || x == vmi::v3fadds              \
        || x == vmi::v3dadds              \
        || x == vmi::v3fsubs              \
        || x == vmi::v3dsubs              \
        || x == vmi::v3fmuls              \
        || x == vmi::v3dmuls              \
        || x == vmi::v3fdivs              \
        || x == vmi::v3ddivs              \
        || x == vmi::v3fmods              \
        || x == vmi::v3dmods              \
        || x == vmi::v4fsets              \
        || x == vmi::v4dsets              \
        || x == vmi::v4fadds              \
        || x == vmi::v4dadds              \
        || x == vmi::v4fsubs              \
        || x == vmi::v4dsubs              \
        || x == vmi::v4fmuls              \
        || x == vmi::v4dmuls              \
        || x == vmi::v4fdivs              \
        || x == vmi::v4ddivs              \
        || x == vmi::v4fmods              \
        || x == vmi::v4dmods              \
    )

#define second_operand_must_be_fpi(x)     \
    (                                     \
        x == vmi::v2fsetsi                \
        || x == vmi::v2dsetsi             \
        || x == vmi::v2faddsi             \
        || x == vmi::v2daddsi             \
        || x == vmi::v2fsubsi             \
        || x == vmi::v2dsubsi             \
        || x == vmi::v2fmulsi             \
        || x == vmi::v2dmulsi             \
        || x == vmi::v2fdivsi             \
        || x == vmi::v2ddivsi             \
        || x == vmi::v2fmodsi             \
        || x == vmi::v2dmodsi             \
        || x == vmi::v3fsetsi             \
        || x == vmi::v3dsetsi             \
        || x == vmi::v3faddsi             \
        || x == vmi::v3daddsi             \
        || x == vmi::v3fsubsi             \
        || x == vmi::v3dsubsi             \
        || x == vmi::v3fmulsi             \
        || x == vmi::v3dmulsi             \
        || x == vmi::v3fdivsi             \
        || x == vmi::v3ddivsi             \
        || x == vmi::v3fmodsi             \
        || x == vmi::v3dmodsi             \
        || x == vmi::v4fsetsi             \
        || x == vmi::v4dsetsi             \
        || x == vmi::v4faddsi             \
        || x == vmi::v4daddsi             \
        || x == vmi::v4fsubsi             \
        || x == vmi::v4dsubsi             \
        || x == vmi::v4fmulsi             \
        || x == vmi::v4dmulsi             \
        || x == vmi::v4fdivsi             \
        || x == vmi::v4ddivsi             \
        || x == vmi::v4fmodsi             \
        || x == vmi::v4dmodsi             \
    )

#define third_operand_must_be_fpr(x)      \
    (                                     \
        x == vmi::fadd                    \
        || x == vmi::fsub                 \
        || x == vmi::fmul                 \
        || x == vmi::fdiv                 \
        || x == vmi::flt                  \
        || x == vmi::flte                 \
        || x == vmi::fgt                  \
        || x == vmi::fgte                 \
        || x == vmi::fcmp                 \
        || x == vmi::fncmp                \
        || x == vmi::dadd                 \
        || x == vmi::dsub                 \
        || x == vmi::dmul                 \
        || x == vmi::ddiv                 \
        || x == vmi::dlt                  \
        || x == vmi::dlte                 \
        || x == vmi::dgt                  \
        || x == vmi::dgte                 \
        || x == vmi::dcmp                 \
        || x == vmi::dncmp                \
    )

#define third_operand_must_be_fpi(x)      \
    (                                     \
        x == vmi::faddi                   \
        || x == vmi::fsubi                \
        || x == vmi::fsubir               \
        || x == vmi::fmuli                \
        || x == vmi::fdivi                \
        || x == vmi::fdivir               \
        || x == vmi::flti                 \
        || x == vmi::fltei                \
        || x == vmi::fgti                 \
        || x == vmi::fgtei                \
        || x == vmi::fcmpi                \
        || x == vmi::fncmpi               \
        || x == vmi::daddi                \
        || x == vmi::dsubi                \
        || x == vmi::dsubir               \
        || x == vmi::dmuli                \
        || x == vmi::ddivi                \
        || x == vmi::ddivir               \
        || x == vmi::dncmpi               \
        || x == vmi::dlti                 \
        || x == vmi::dltei                \
        || x == vmi::dgti                 \
        || x == vmi::dgtei                \
        || x == vmi::dcmpi                \
        || x == vmi::dncmpi               \
    )

#define decode_instr ((vmi)(m_code >> instr_shift))
#define check_flag(f) (((m_code | flag_mask) ^ flag_mask) & f)
#define set_flag(f) (m_code |= f)

#define vm_instr_assigns_r1(i) ((i >= vm_instruction::ld8 && i <= vm_instruction::ld64) || (i >= vm_instruction::add && i <= vm_instruction::srir) || i == vm_instruction::mptr)
#define vm_instr_assigns_r2(i) ((i == vm_instruction::mtfp || i == vm_instruction::mffp))

#define vm_reg_is_volatile(r) (r >= vm_register::v0 && r <= vm_register::vf3)
#define vm_reg_is_arg(r) (r >= vm_register::a0 && r <= vm_register::fa7)
#define is_fpr(x) ((x >= vmr::f0 && x <= vmr::f15) || (x >= vmr::fa0 && x <= vmr::fa7) || (x >= vmr::vf0 && x <= vmr::vf3))