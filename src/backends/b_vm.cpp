#include <backends/vm.h>
#include <backends/register_allocator.h>
#include <compiler/tac.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/context.h>
#include <bind/bind.h>

namespace gjs {
    #define is_fpr(x) (x >= vmr::f0 && x <= vmr::f15)

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

    void vm_backend::generate(compilation_output& in) {
        for (u16 f = 0;f < in.funcs.size();f++) {
            gen_function(in, f);
            m_ctx->add(in.funcs[f].func);
        }
    }
    
    void vm_backend::gen_function(compilation_output& in, u16 fidx) {
        using op = compile::operation;
        using vmi = vm_instruction;
        using vmr = vm_register;
        using var = compile::var;

        robin_hood::unordered_map<vmr, u64> reg_stack_addrs;
        robin_hood::unordered_map<u64, u64> stack_stack_addrs;
        robin_hood::unordered_map<u64, u64> tac_map;
        std::vector<var> params;

        u64 out_begin = m_instructions.size();
        for (u32 c = in.funcs[fidx].begin;c <= in.funcs[fidx].end;c++) {
            tac_map[c] = m_instructions.size();
            const compile::tac_instruction& i = in.code[c];
            const var& o1 = i.operands[0];
            const var& o2 = i.operands[1];
            const var& o3 = i.operands[2];

            vmr r1;
            vmr r2;
            vmr r3;
            script_type* t1 = o1.valid() ? o1.type() : nullptr;
            script_type* t2 = o2.valid() ? o2.type() : nullptr;
            script_type* t3 = o3.valid() ? o3.type() : nullptr;
            
            auto arith = [&](vmi rr, vmi ri, vmi ir) {
                if (o3.is_imm()) {
                    if (t3->is_floating_point) {
                        if (t3->size == sizeof(f64)) m_instructions += encode(ri).operand(r1).operand(r2).operand(o3.imm_d());
                        else m_instructions += encode(ri).operand(r1).operand(r2).operand(o3.imm_f());
                    } else {
                        if (t3->is_unsigned) m_instructions += encode(ri).operand(r1).operand(r2).operand(o3.imm_u());
                        else m_instructions += encode(ri).operand(r1).operand(r2).operand(o3.imm_i());
                    }
                }
                else if (o2.is_imm()) {
                    if (t2->is_floating_point) {
                        if (t2->size == sizeof(f64)) m_instructions += encode(ir).operand(r1).operand(r3).operand(o2.imm_d());
                        else m_instructions += encode(ir).operand(r1).operand(r3).operand(o2.imm_f());
                    } else {
                        if (t2->is_unsigned) m_instructions += encode(ir).operand(r1).operand(r3).operand(o2.imm_u());
                        else m_instructions += encode(ir).operand(r1).operand(r3).operand(o2.imm_i());
                    }
                } else m_instructions += encode(rr).operand(r1).operand(r2).operand(r3);
                m_map.append(i.src);

                if (o1.is_spilled()) {
                    u8 sz = t1->size;
                    if (!t1->is_primitive) sz = sizeof(void*);
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
                    m_instructions += encode(st).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                }
            };

            if (i.op != op::param) {
                if (o1.is_spilled()) {
                    if (!compile::is_assignment(i.op)) {
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
                        m_map.append(i.src);
                    } else r1 = vmr::v0;
                } else if (o1.valid() && !o1.is_imm()) {
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
                    m_map.append(i.src);
                } else if (o2.valid() && !o2.is_imm()) {
                    r2 = vmr((u8)vmr::s0 + o2.reg_id());
                    if (t2->is_floating_point) r2 = vmr((u8)vmr::f0 + o2.reg_id());
                }

                if (o3.is_spilled()) {
                    r3 = vmr::v2;
                    if (t3->is_floating_point) r3 = vmr::f15;

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
                    m_map.append(i.src);
                } else if (o3.valid() && !o3.is_imm()) {
                    r3 = vmr((u8)vmr::s0 + o3.reg_id());
                    if (t3->is_floating_point) r3 = vmr((u8)vmr::f0 + o3.reg_id());
                }
            }

            // This comment was written in a police station while waiting for a ride
            switch(i.op) {
                case op::null: {
                    m_instructions += encode(vmi::null);
                    m_map.append(i.src);
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
                    m_map.append(i.src);
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

                    m_instructions += encode(st).operand(r2).operand(r1).operand((u64)0);
                    m_map.append(i.src);
                    break;
                }
                case op::stack_alloc: {
                    u64 addr = in.funcs[fidx].stack.alloc(o2.imm_u());
                    if (o1.is_spilled()) {
                        stack_stack_addrs[o1.stack_off()] = addr;
                        m_instructions += encode(vmi::addui).operand(r1).operand(vmr::sp).operand(addr);
                        m_map.append(i.src);
                        m_instructions += encode(vmi::st64).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                    } else {
                        reg_stack_addrs[r1] = addr;
                        m_instructions += encode(vmi::addui).operand(r1).operand(vmr::sp).operand(addr);
                    }
                    m_map.append(i.src);
                    break;
                }
                case op::stack_free: {
                    if (o1.is_spilled()) {
                        auto it = stack_stack_addrs.find(o1.stack_off());
                        in.funcs[fidx].stack.free(it->getSecond());
                        stack_stack_addrs.erase(it);
                    } else {
                        auto it = reg_stack_addrs.find(r1);
                        in.funcs[fidx].stack.free(it->getSecond());
                        reg_stack_addrs.erase(it);
                    }
                    break;
                }
                case op::spill: {
                    // probably not a necessary TAC instruction
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
                    vmi rr, ri, ir;
                    if (t1->is_floating_point) {
                        if (t1->size == sizeof(f64)) { rr = vmi::dadd; ri = vmi::daddi; ir = vmi::daddi; }
                        else { rr = vmi::fadd; ri = vmi::faddi; ir = vmi::faddi; }
                    } else {
                        if (t1->is_unsigned || !t1->is_primitive) { rr = vmi::addu; ri = vmi::addui; ir = vmi::addui; }
                        else { rr = vmi::add; ri = vmi::addi; ir = vmi::addi; }
                    }

                    if (o2.is_imm()) {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) m_instructions += encode(ri).operand(r1).operand(vmr::zero).operand(o2.imm_d());
                            else m_instructions += encode(ri).operand(r1).operand(vmr::zero).operand(o2.imm_f());
                        } else {
                            if (t2->is_unsigned) m_instructions += encode(ri).operand(r1).operand(vmr::zero).operand(o2.imm_u());
                            else m_instructions += encode(ri).operand(r1).operand(vmr::zero).operand(o2.imm_i());
                        }
                    }
                    else if (o1.is_imm()) {
                        if (t1->is_floating_point) {
                            if (t1->size == sizeof(f64)) m_instructions += encode(ir).operand(r1).operand(vmr::zero).operand(o1.imm_d());
                            else m_instructions += encode(ir).operand(r1).operand(vmr::zero).operand(o1.imm_f());
                        } else {
                            if (t1->is_unsigned) m_instructions += encode(ir).operand(r1).operand(vmr::zero).operand(o1.imm_u());
                            else m_instructions += encode(ir).operand(r1).operand(vmr::zero).operand(o1.imm_i());
                        }
                    } else m_instructions += encode(rr).operand(r1).operand(r2).operand(vmr::zero);
                    m_map.append(i.src);
                    break;
                }
                case op::neg: {
                    // todo
                    break;
                }
                case op::call: {
                    // Backup registers that were populated before this call
                    // and will be used after this call
                    struct bk { vmr reg; u64 addr; u64 sz; };
                    std::vector<bk> backup;
                    auto live = in.regs.get_live(c);
                    for (u16 l = 0;l < live.size();l++) {
                        if (live[l].begin == c) continue; // return value register does not need to be backed up
                        if (live[l].end > c && !live[l].is_stack()) {
                            script_type* tp = in.code[live[l].begin].operands[0].type();

                            u8 sz = tp->size;
                            if (!tp->is_primitive) sz = sizeof(void*);
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
                            vmr reg = live[l].is_fp ? vmr(u32(vmr::f0) + live[l].reg_id) : vmr(u32(vmr::s0) + live[l].reg_id);
                            u64 sl = in.funcs[fidx].stack.alloc(sz);

                            if (live[l].is_fp) backup.push_back({ reg, sl, sz });
                            else backup.push_back({ reg, sl, sz });

                            m_instructions += encode(st).operand(reg).operand(vmr::sp).operand(sl);
                            m_map.append(i.src);
                        }
                    }

                    // pass parameters
                    std::vector<u8> passed;
                    while (passed.size() < params.size()) {
                        for (u8 p = 0;p < params.size();p++) {
                            bool was_passed = false;
                            for (u8 p1 = 0;p1 < passed.size() && !was_passed;p1++) was_passed = passed[p1] == p;
                            if (was_passed) continue;

                            // determine if passing the parameter will overwrite another unpassed parameter
                            bool will_overwrite = false;
                            for (u8 p1 = 0;p1 < params.size() && !will_overwrite;p1++) {
                                if (p1 == p) continue;
                                bool was_passed = false;
                                for (u8 p2 = 0;p2 < passed.size() && !was_passed;p2++) was_passed = passed[p2] == p1;
                                if (was_passed || params[p1].is_spilled() || params[p1].is_imm()) continue;

                                vmr reg = params[p1].type()->is_floating_point ? vmr(u32(vmr::f0) + params[p1].reg_id()) : vmr(u32(vmr::s0) + params[p1].reg_id());
                                will_overwrite = i.callee->signature.arg_locs[p] == reg;
                            }

                            if (will_overwrite) continue;
                            passed.push_back(p);

                            // pass the parameter
                            script_type* tp = params[p].type();
                            if (params[p].is_imm()) {
                                if (tp->is_floating_point) {
                                    if (!is_fpr(i.callee->signature.arg_locs[p]) && i.callee->is_method_of && i.callee->is_method_of->requires_subtype) {
                                        // fp value must be passed through GP register
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::daddi).operand(vmr::f15).operand(vmr::zero).operand(params[p].imm_d());
                                        else m_instructions += encode(vmi::faddi).operand(vmr::f15).operand(vmr::zero).operand(params[p].imm_f());
                                        m_map.append(i.src);
                                        m_instructions += encode(vmi::mffp).operand(vmr::f15).operand(i.callee->signature.arg_locs[p]);
                                    } else {
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::daddi).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(params[p].imm_d());
                                        else m_instructions += encode(vmi::faddi).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(params[p].imm_f());
                                    }
                                } else {
                                    m_instructions += encode(vmi::addui).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(params[p].imm_u());
                                }
                                m_map.append(i.src);
                            } else if (params[p].is_spilled()) {
                                vmi ld = vmi::ld8;
                                u16 sz = tp->is_primitive ? tp->size : sizeof(void*);
                                switch (sz) {
                                    case 2: { ld = vmi::ld16; break; }
                                    case 4: { ld = vmi::ld32; break; }
                                    case 8: { ld = vmi::ld64; break; }
                                    default: {
                                        // invalid size
                                        // exception
                                    }
                                }
                                m_instructions += encode(ld).operand(i.callee->signature.arg_locs[p]).operand(vmr::sp).operand((u64)params[p].stack_off());
                                m_map.append(i.src);
                            } else {
                                vmr reg = tp->is_floating_point ? vmr(u32(vmr::f0) + params[p].reg_id()) : vmr(u32(vmr::s0) + params[p].reg_id());
                                if (tp->is_floating_point) {
                                    if (!is_fpr(i.callee->signature.arg_locs[p]) && i.callee->is_method_of && i.callee->is_method_of->requires_subtype) {
                                        // fp value must be passed through GP register
                                        m_instructions += encode(vmi::mffp).operand(reg).operand(i.callee->signature.arg_locs[p]);
                                    } else {
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::dadd).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(reg);
                                        else m_instructions += encode(vmi::fadd).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(reg);
                                    }
                                } else {
                                    m_instructions += encode(vmi::addu).operand(i.callee->signature.arg_locs[p]).operand(vmr::zero).operand(reg);
                                }
                                m_map.append(i.src);
                            }
                        }
                    }

                    params.clear();

                    // backup $ra
                    u64 ra = in.funcs[fidx].stack.alloc(sizeof(u64));
                    m_instructions += encode(vmi::st64).operand(vmr::ra).operand(vmr::sp).operand(ra);
                    m_map.append(i.src);

                    // offset $sp
                    m_instructions += encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand(in.funcs[fidx].stack.size());
                    m_map.append(i.src);

                    // do the call
                    if (i.callee->is_host) m_instructions += encode(vmi::jal).operand(i.callee->access.wrapped->address);
                    else m_instructions += encode(vmi::jal).operand(i.callee->access.entry);
                    m_map.append(i.src);

                    // restore $sp
                    m_instructions += encode(vmi::subui).operand(vmr::sp).operand(vmr::sp).operand(in.funcs[fidx].stack.size());
                    m_map.append(i.src);

                    // store return value if necessary
                    if (t1) {
                        if (i.callee->signature.returns_pointer) {
                            m_instructions += encode(vmi::addu).operand(r1).operand(i.callee->signature.return_loc).operand(vmr::zero);
                        } else {
                            if (t1->is_floating_point) {
                                if (!is_fpr(i.callee->signature.return_loc) && i.callee->is_method_of && i.callee->is_method_of->requires_subtype) {
                                    // fp value returned through gp register
                                    m_instructions += encode(vmi::mtfp).operand(i.callee->signature.return_loc).operand(r1);
                                } else {
                                    if (t1->size == sizeof(f64)) m_instructions += encode(vmi::dadd).operand(r1).operand(vmr::zero).operand(i.callee->signature.return_loc);
                                    else m_instructions += encode(vmi::fadd).operand(r1).operand(vmr::zero).operand(i.callee->signature.return_loc);
                                }
                            } else {
                                m_instructions += encode(vmi::addu).operand(r1).operand(i.callee->signature.return_loc).operand(vmr::zero);
                            }
                        }
                        m_map.append(i.src);

                        if (o1.is_spilled()) {
                            u8 sz = t1->size;
                            if (!t1->is_primitive) sz = sizeof(void*);
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
                            m_instructions += encode(st).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                            m_map.append(i.src);
                        }
                    }

                    // restore $ra
                    in.funcs[fidx].stack.free(ra);
                    m_instructions += encode(vmi::ld64).operand(vmr::ra).operand(vmr::sp).operand(ra);
                    m_map.append(i.src);

                    // restore backed up registers
                    for (u16 b = 0;b < backup.size();b++) {
                        vmi ld = vmi::ld8;
                        switch (backup[b].sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: {
                                // invalid size
                                // exception
                            }
                        }

                        m_instructions += encode(ld).operand(backup[b].reg).operand(vmr::sp).operand(backup[b].addr);
                        m_map.append(i.src);
                        in.funcs[fidx].stack.free(backup[b].addr);
                    }

                    break;
                }
                case op::param: {
                    params.push_back(o1);
                    break;
                }
                case op::ret: {
                    m_instructions += encode(vmi::jmpr).operand(vmr::ra);
                    m_map.append(i.src);
                    break;
                }
                case op::branch: {
                    // todo: Why did I add the b* branch instructions if only bneqz is used
                    m_instructions += encode(vmi::bneqz).operand(r1).operand(o2.imm_u());
                    m_map.append(i.src);
                    break;
                }
                case op::jump: {
                    if (o1.is_imm()) m_instructions += encode(vmi::jmp).operand(o1.imm_u());
                    else m_instructions += encode(vmi::jmpr).operand(r1);
                    m_map.append(i.src);
                    break;
                }
            }
        }
        
        // update jump, branch, jal addresses
        for (u64 c = out_begin;c < m_instructions.size();c++) {
            switch (m_instructions[c].instr()) {
                case vmi::jal:
                case vmi::jmp:
                case vmi::beqz:
                case vmi::bneqz:
                case vmi::bgtz:
                case vmi::bgtez:
                case vmi::bltz:
                case vmi::bltez: {
                    u64 addr = m_instructions[c].imm_u();
                    if (addr < m_instructions.size()) {
                        // jumps to VM code
                        m_instructions[c].m_imm = tac_map[addr];
                    }
                }
                default: continue;
            }
        }

        in.funcs[fidx].func->access.entry = out_begin;
    }
    
    u16 vm_backend::gp_count() const {
        return 8;
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