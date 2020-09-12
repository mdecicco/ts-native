#include <backends/vm.h>
#include <compiler/tac.h>
#include <common/script_type.h>
#include <backends/register_allocator.h>

namespace gjs {
    vm_exception::vm_exception(vm_backend* ctx, const std::string& _text) :
        text(_text), raised_from_script(true), line(0), col(0)
    {
        if (ctx->is_executing()) {
            source_ref info = ctx->map()->get((address)ctx->state()->registers[(integer)vm_register::ip]);
            file = info.filename;
            lineText = info.line_text;
            line = info.line;
            col = info.col;
        }
        else raised_from_script = false;
    }

    vm_exception::vm_exception(const std::string& _text) : text(_text), raised_from_script(false), line(0), col(0) {
    }

    vm_exception::~vm_exception() {
    }


    vm_backend::vm_backend(vm_allocator* alloc, u32 stack_size, u32 mem_size) :
        m_vm(this, alloc, stack_size, mem_size), m_instructions(alloc), m_is_executing(false),
        m_log_instructions(false), m_alloc(alloc)
    {
        m_instructions += encode(vm_instruction::term);
        m_map.append("[internal code]", "", 0, 0);
    }

    vm_backend::~vm_backend() {
    }

    void vm_backend::execute(address entry) {
        m_is_executing = true;

        m_vm.execute(m_instructions, entry);

        m_is_executing = false;
    }

    void vm_backend::generate(const compilation_output& in) {
        using op = compile::operation;
        using vmi = vm_instruction;
        using vmr = vm_register;
        using var = compile::var;

        for (u32 c = 0;c < in.code.size();c++) {
            const compile::tac_instruction& i = in.code[c];
            const var& o1 = i.operands[0];
            const var& o2 = i.operands[1];
            const var& o3 = i.operands[2];

            vmr r1;
            vmr r2;
            vmr r3;
            script_type* t1 = o1.type();
            script_type* t2 = o2.type();
            script_type* t3 = o3.type();
            
            auto arith = [&](vmi rr, vmi ri, vmi ir) {
                if (o3.is_imm()) {
                    if (t3->is_floating_point) {
                        if (t3->size == sizeof(f64)) m_instructions += encode(ri).operand(r1).operand(r2).operand((i64)o3.imm_d());
                        else m_instructions += encode(ri).operand(r1).operand(r2).operand((i64)o3.imm_f());
                    } else {
                        if (t3->is_unsigned) m_instructions += encode(ri).operand(r1).operand(r2).operand((i64)o3.imm_u());
                        else m_instructions += encode(ri).operand(r1).operand(r2).operand(o3.imm_i());
                    }
                }
                else if (o2.is_imm()) {
                    if (t2->is_floating_point) {
                        if (t2->size == sizeof(f64)) m_instructions += encode(ir).operand(r1).operand(r3).operand((i64)o2.imm_d());
                        else m_instructions += encode(ir).operand(r1).operand(r3).operand((i64)o2.imm_f());
                    } else {
                        if (t2->is_unsigned) m_instructions += encode(ir).operand(r1).operand(r3).operand((i64)o2.imm_u());
                        else m_instructions += encode(ir).operand(r1).operand(r3).operand(o2.imm_i());
                    }
                } else m_instructions += encode(rr).operand(r1).operand(r2).operand(r3);
            };

            if (o1.is_spilled()) {
                r1 = vmr::v0;
                if (t1->is_floating_point) r1 = vmr::f13;

                u8 sz = t1->size;
                if (!t1->is_primitive) sz = sizeof(void*);
                vmi ld = vmi::ld8;
                switch (sz) {
                    case 2: { ld = vmi::ld16; break; }
                    case 4: { ld = vmi::ld32; break; }
                    case 8: { ld = vmi::ld64; break; }
                    default: {
                        // invalid size
                        // exception
                    }
                }
                m_instructions += encode(ld).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
            } else if (!o1.is_imm()) {
                r1 = vmr((u8)vmr::s0 + o1.reg_id());
                if (t1->is_floating_point) r1 = vmr((u8)vmr::f0 + o1.reg_id());
            }

            if (o2.is_spilled()) {
                r2 = vmr::v1;
                if (t2->is_floating_point) r2 = vmr::f14;

                u8 sz = t2->size;
                if (!t2->is_primitive) sz = sizeof(void*);
                vmi ld = vmi::ld8;
                switch (sz) {
                    case 2: { ld = vmi::ld16; break; }
                    case 4: { ld = vmi::ld32; break; }
                    case 8: { ld = vmi::ld64; break; }
                    default: {
                        // invalid size
                        // exception
                    }
                }
                m_instructions += encode(ld).operand(r2).operand(vmr::sp).operand((u64)o2.stack_off());
            } else if (!o2.is_imm()) {
                r2 = vmr((u8)vmr::s0 + o2.reg_id());
                if (t2->is_floating_point) r2 = vmr((u8)vmr::f0 + o2.reg_id());
            }

            if (o3.is_spilled()) {
                r3 = vmr::v2;
                if (t3->is_floating_point) r3 = vmr::f14;

                u8 sz = t3->size;
                if (!o3.type()->is_primitive) sz = sizeof(void*);
                vmi ld = vmi::ld8;
                switch (sz) {
                    case 2: { ld = vmi::ld16; break; }
                    case 4: { ld = vmi::ld32; break; }
                    case 8: { ld = vmi::ld64; break; }
                    default: {
                        // invalid size
                        // exception
                    }
                }
                m_instructions += encode(ld).operand(r3).operand(vmr::sp).operand((u64)o3.stack_off());
            } else if (!o3.is_imm()) {
                r3 = vmr((u8)vmr::s0 + o3.reg_id());
                if (t3->is_floating_point) r3 = vmr((u8)vmr::f0 + o3.reg_id());
            }

            switch(i.op) {
                case op::null: {
                    m_instructions += encode(vmi::null);
                    break;
                }
                case op::load: {
                    u8 sz = o1.type()->size;
                    if (!o1.type()->is_primitive) sz = sizeof(void*);
                    vmi ld = vmi::ld8;
                    switch (sz) {
                        case 2: { ld = vmi::ld16; break; }
                        case 4: { ld = vmi::ld32; break; }
                        case 8: { ld = vmi::ld64; break; }
                        default: {
                            // invalid size
                            // exception
                        }
                    }

                    m_instructions += encode(ld).operand(r1).operand(r2).operand((u64)0);
                    break;
                }
                case op::store: {
                    u8 sz = o1.type()->size;
                    if (!o1.type()->is_primitive) sz = sizeof(void*);
                    vmi st = vmi::st8;
                    switch (sz) {
                        case 2: { st = vmi::st16; break; }
                        case 4: { st = vmi::st32; break; }
                        case 8: { st = vmi::st64; break; }
                        default: {
                            // invalid size
                            // exception
                        }
                    }

                    m_instructions += encode(st).operand(r1).operand(r2).operand((u64)0);
                    break;
                }
                case op::stack_alloc: {
                    break;
                }
                case op::stack_free: {
                    break;
                }
                case op::spill: {
                    break;
                }
                case op::iadd: {
                    arith(vmi::add, vmi::addi, vmi::addi);
                    break;
                }
                case op::isub: {
                    arith(vmi::sub, vmi::subi, vmi::subir);
                    break;
                }
                case op::imul: {
                    arith(vmi::mul, vmi::muli, vmi::muli);
                    break;
                }
                case op::idiv: {
                    arith(vmi::div, vmi::divi, vmi::divir);
                    break;
                }
                case op::imod: {
                    break;
                }
                case op::uadd: {
                    arith(vmi::addu, vmi::addui, vmi::addui);
                    break;
                }
                case op::usub: {
                    arith(vmi::subu, vmi::subui, vmi::subuir);
                    break;
                }
                case op::umul: {
                    arith(vmi::mulu, vmi::mului, vmi::mului);
                    break;
                }
                case op::udiv: {
                    arith(vmi::divu, vmi::divui, vmi::divuir);
                    break;
                }
                case op::umod: {
                    break;
                }
                case op::fadd: {
                    arith(vmi::fadd, vmi::faddi, vmi::faddi);
                    break;
                }
                case op::fsub: {
                    arith(vmi::fsub, vmi::fsubi, vmi::fsubir);
                    break;
                }
                case op::fmul: {
                    arith(vmi::fmul, vmi::fmuli, vmi::fmuli);
                    break;
                }
                case op::fdiv: {
                    arith(vmi::fdiv, vmi::fdivi, vmi::fdivir);
                    break;
                }
                case op::fmod: {
                    break;
                }
                case op::dadd: {
                    arith(vmi::dadd, vmi::daddi, vmi::daddi);
                    break;
                }
                case op::dsub: {
                    arith(vmi::dsub, vmi::dsubi, vmi::dsubir);
                    break;
                }
                case op::dmul: {
                    arith(vmi::dmul, vmi::dmuli, vmi::dmuli);
                    break;
                }
                case op::ddiv: {
                    arith(vmi::ddiv, vmi::ddivi, vmi::ddivir);
                    break;
                }
                case op::dmod: {
                    break;
                }
                case op::sl: {
                    arith(vmi::sl, vmi::sli, vmi::slir);
                    break;
                }
                case op::sr: {
                    arith(vmi::sr, vmi::sri, vmi::srir);
                    break;
                }
                case op::land: {
                    arith(vmi::and, vmi::andi, vmi::andi);
                    break;
                }
                case op::lor: {
                    arith(vmi::or, vmi::ori, vmi::ori);
                    break;
                }
                case op::band: {
                    arith(vmi::band, vmi::bandi, vmi::bandi);
                    break;
                }
                case op::bor: {
                    arith(vmi::bor, vmi::bori, vmi::bori);
                    break;
                }
                case op::bxor: {
                    arith(vmi::xor, vmi::xori, vmi::xori);
                    break;
                }
                case op::ilt: {
                    arith(vmi::lt, vmi::lti, vmi::gtei);
                    break;
                }
                case op::igt: {
                    arith(vmi::gt, vmi::gti, vmi::ltei);
                    break;
                }
                case op::ilte: {
                    arith(vmi::lte, vmi::ltei, vmi::gti);
                    break;
                }
                case op::igte: {
                    arith(vmi::gte, vmi::gtei, vmi::lt);
                    break;
                }
                case op::incmp: {
                    arith(vmi::ncmp, vmi::ncmpi, vmi::ncmpi);
                    break;
                }
                case op::icmp: {
                    arith(vmi::cmp, vmi::cmpi, vmi::cmpi);
                    break;
                }
                case op::ult: {
                    arith(vmi::lt, vmi::lti, vmi::gtei);
                    break;
                }
                case op::ugt: {
                    arith(vmi::gt, vmi::gti, vmi::ltei);
                    break;
                }
                case op::ulte: {
                    arith(vmi::lte, vmi::ltei, vmi::gti);
                    break;
                }
                case op::ugte: {
                    arith(vmi::gte, vmi::gtei, vmi::lt);
                    break;
                }
                case op::uncmp: {
                    arith(vmi::ncmp, vmi::ncmpi, vmi::ncmpi);
                    break;
                }
                case op::ucmp: {
                    arith(vmi::cmp, vmi::cmpi, vmi::cmpi);
                    break;
                }
                case op::flt: {
                    arith(vmi::flt, vmi::flti, vmi::fgtei);
                    break;
                }
                case op::fgt: {
                    arith(vmi::fgt, vmi::fgti, vmi::fltei);
                    break;
                }
                case op::flte: {
                    arith(vmi::flte, vmi::fltei, vmi::fgti);
                    break;
                }
                case op::fgte: {
                    arith(vmi::fgte, vmi::fgtei, vmi::flt);
                    break;
                }
                case op::fncmp: {
                    arith(vmi::fncmp, vmi::fncmpi, vmi::fncmpi);
                    break;
                }
                case op::fcmp: {
                    arith(vmi::fcmp, vmi::fcmpi, vmi::fcmpi);
                    break;
                }
                case op::dlt: {
                    arith(vmi::dlt, vmi::dlti, vmi::dgtei);
                    break;
                }
                case op::dgt: {
                    arith(vmi::dgt, vmi::dgti, vmi::dltei);
                    break;
                }
                case op::dlte: {
                    arith(vmi::dlte, vmi::dltei, vmi::dgti);
                    break;
                }
                case op::dgte: {
                    arith(vmi::dgte, vmi::dgtei, vmi::dlt);
                    break;
                }
                case op::dncmp: {
                    arith(vmi::dncmp, vmi::dncmpi, vmi::dncmpi);
                    break;
                }
                case op::dcmp: {
                    arith(vmi::dcmp, vmi::dcmpi, vmi::dcmpi);
                    break;
                }
                case op::eq: {
                    break;
                }
                case op::neg: {
                    break;
                }
                case op::call: {
                    break;
                }
                case op::param: {
                    break;
                }
                case op::ret: {
                    break;
                }
                case op::branch: {
                    break;
                }
                case op::jump: {
                    break;
                }
            }
        }
    }
    
    u16 vm_backend::gp_count() const {
        return 3;
    }

    u16 vm_backend::fp_count() const {
        // f13, f14, f15 used for
        // loading spilled values
        return 13;
    }

    void vm_backend::call(script_function* func, void* ret, void** args) {
        /*
        if (sizeof...(args) != signature.arg_locs.size()) {
            throw vm_exception(format(
                "Function '%s' takes %d arguments, %d %s provided",
                name.c_str(),
                signature.arg_locs.size(),
                sizeof...(args),
                (sizeof...(args)) == 1 ? "was" : "were"
            ));
        }

        if (signature.arg_locs.size() > 0) bind::set_arguments(m_ctx, this, 0, args...);
        if (is_host) m_ctx->vm()->call_external(access.wrapped->address);
        else m_ctx->vm()->execute(*m_ctx->code(), access.entry);

        if (signature.return_type->size != 0) {
            nd::from_reg(m_ctx, result, &m_ctx->state()->registers[(u8)signature.return_loc]);
        }
        */
    }
};