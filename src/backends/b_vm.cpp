#include <gjs/backends/b_vm.h>
#include <gjs/backends/register_allocator.h>
#include <gjs/compiler/tac.h>
#include <gjs/gjs.hpp>

namespace gjs {
    vm_backend::vm_backend(vm_allocator* alloc, u32 stack_size, u32 mem_size) :
        m_vm(this, alloc, stack_size, mem_size), m_instructions(alloc), m_execution_level(0),
        m_log_instructions(false), m_alloc(alloc), m_log_lines(false)
    {
        m_instructions += encode(vm_instruction::term);
        m_map.append("[internal code]", "", 0, 0);
    }

    vm_backend::~vm_backend() {
    }

    void vm_backend::execute(address entry) {
        m_execution_level++;

        try {
            m_vm.execute(m_instructions, entry, m_execution_level > 1);
        } catch (const error::vm_exception& e) {
            m_execution_level--;
            throw e;
        } catch (const error::runtime_exception& e) {
            m_execution_level--;
            throw e;
        } catch (const error::exception& e) {
            m_execution_level--;
            throw e;
        } catch (const std::exception& e) {
            m_execution_level--;
            throw e;
        }

        m_execution_level--;
    }

    void vm_backend::generate(compilation_output& in) {
        tac_map tmap;

        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            gen_function(in, tmap, f);
        }
    }
    
    void vm_backend::gen_function(compilation_output& in, tac_map& tmap, u16 fidx) {
        using op = compile::operation;
        using vmi = vm_instruction;
        using vmr = vm_register;
        using var = compile::var;

        robin_hood::unordered_map<u64, u64> reg_stack_addrs;
        robin_hood::unordered_map<u64, u64> stack_stack_addrs;
        std::vector<var> params;
        script_function* cf = in.funcs[fidx].func;

        u64 out_begin = m_instructions.size();
        auto& code = in.funcs[fidx].code;
        robin_hood::unordered_map<label_id, address> label_map;
        robin_hood::unordered_map<label_id, address> ir_label_map;
        struct deferred_label { address idx; label_id label; }; // label == 0 indicates a jump to the function epilogue
        std::vector<deferred_label> deferred_label_instrs;

        // determine registers that need to be handled by the prologue and epilogue
        robin_hood::unordered_flat_set<vm_register> assigned_non_volatile;
        // also the end of the lifetimes of the argument registers
        robin_hood::unordered_map<vm_register, address> arg_lifetime_ends;

        for (u64 i = 0;i < code.size();i++) {
            if (code[i].op == compile::operation::label) ir_label_map[code[i].labels[0]] = i;
        }

        for (u64 c = 0;c < code.size();c++) {
            auto& i = code[c];

            for (u8 o = 0;o < 3;o++) {
                if (i.operands[o].valid() && i.operands[o].is_arg()) {
                    auto& arg = cf->type->signature->args[i.operands[o].arg_idx()];
                    bool isFP = arg.tp->is_floating_point;
                    arg_lifetime_ends[arg.loc] = c;
                }
            }

            // If a backwards jump goes into a live range,
            // then that live range must be extended to fit
            // the jump (if it doesn't already)
            u64 jaddr = u64(-1);

            if (i.op == compile::operation::jump) jaddr = ir_label_map[i.labels[0]];
            else if (i.op == compile::operation::branch) jaddr = ir_label_map[i.labels[0]];

            if (jaddr != u64(-1)) {
                if (jaddr > c) continue;

                for (auto& r : arg_lifetime_ends) {
                    if (r.getSecond() >= jaddr && r.getSecond() < c) {
                        r.second = c;
                    }
                }
            }

            if (compile::is_assignment(i)) {
                u8 assignedOpIdx = 0;
                vmr r[3];
                r[0] = r[1] = r[2] = vmr::register_count;
                const var& o1 = i.operands[0];
                const var& o2 = i.operands[1];
                const var& o3 = i.operands[2];
                script_type* t1 = o1.valid() ? o1.type() : nullptr;
                script_type* t2 = o2.valid() ? o2.type() : nullptr;
                script_type* t3 = o3.valid() ? o3.type() : nullptr;

                if (o1.is_spilled()) {
                    if (!compile::is_assignment(i)) {
                        r[0] = vmr::v0;
                        if (t1->is_floating_point) r[0] = vmr::vf0;
                    } else r[0] = vmr::v0;
                } else if (o1.valid() && !o1.is_imm()) {
                    if (o1.is_arg()) {
                        r[0] = in.funcs[fidx].func->type->signature->args[o1.arg_idx()].loc;
                    } else {
                        r[0] = vmr((u8)vmr::s0 + o1.reg_id());
                        if (t1->is_floating_point) r[0] = vmr((u8)vmr::f0 + o1.reg_id());
                    }
                }

                if (o2.is_spilled()) {
                    r[1] = vmr::v1;
                    if (t2->is_floating_point) r[1] = vmr::vf1;
                } else if (o2.valid() && !o2.is_imm()) {
                    if (o2.is_arg()) {
                        r[1] = in.funcs[fidx].func->type->signature->args[o2.arg_idx()].loc;
                    } else {
                        r[1] = vmr((u8)vmr::s0 + o2.reg_id());
                        if (t2->is_floating_point) r[1] = vmr((u8)vmr::f0 + o2.reg_id());
                    }
                }

                if (o3.is_spilled()) {
                    r[2] = vmr::v2;
                    if (t3->is_floating_point) r[2] = vmr::vf2;
                } else if (o3.valid() && !o3.is_imm()) {
                    if (o3.is_arg()) {
                        r[2] = in.funcs[fidx].func->type->signature->args[o3.arg_idx()].loc;
                    } else {
                        r[2] = vmr((u8)vmr::s0 + o3.reg_id());
                        if (t3->is_floating_point) r[2] = vmr((u8)vmr::f0 + o3.reg_id());
                    }
                }

                if (i.op == compile::operation::call) {
                    assigned_non_volatile.insert(vmr::ra);
                    function_signature* sig = nullptr;
                    if (i.callee) sig = i.callee->type->signature;
                    else {
                        sig = i.callee_v.type()->signature;
                        assignedOpIdx = 1;
                    }

                    for (u8 a = 0;a < sig->args.size();a++) {
                        bool alreadyOwned = false;
                        for (u8 a1 = 0;a1 < cf->type->signature->args.size();a1++) {
                            if (sig->args[a].loc == cf->type->signature->args[a1].loc) {
                                alreadyOwned = true;
                                break;
                            }
                        }
                        // caller of this function already backed up this register
                        if (alreadyOwned) continue;
                        assigned_non_volatile.insert(sig->args[a].loc);
                    }

                    if (sig->return_type->size == 0) continue;
                }

                if (!vm_reg_is_volatile(r[assignedOpIdx]) && r[assignedOpIdx] != cf->type->signature->return_loc) {
                    assigned_non_volatile.insert(r[assignedOpIdx]);
                }
            }
        }

        // generate function prologue
        struct prologueInfo { vmr reg; u64 addr; };
        std::vector<prologueInfo> prologue;
        for (auto r : assigned_non_volatile) {
            u64 addr = in.funcs[fidx].stack.alloc(sizeof(u64));
            prologue.push_back({ r, addr });
            m_instructions += encode(vmi::st64).operand(r).operand(vmr::sp).operand(addr);
            m_map.append(in.funcs[fidx].ref);
        }

        for (u64 c = 0;c < code.size();c++) {
            tmap[c] = m_instructions.size();
            const compile::tac_instruction& i = code[c];
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
                        default: { break; }
                    }
                    m_instructions += encode(st).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                    m_map.append(i.src);
                }
            };

            if (i.op != op::param) {
                if (o1.is_spilled()) {
                    if (!compile::is_assignment(i)) {
                        r1 = vmr::v0;
                        if (t1->is_floating_point) r1 = vmr::vf0;

                        u8 sz = t1->size;
                        if (!t1->is_primitive) sz = sizeof(void*);
                        vmi ld = vmi::ld8;
                        switch (sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: { break; }
                        }
                        m_instructions += encode(ld).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                        m_map.append(i.src);
                    } else r1 = vmr::v0;
                } else if (o1.valid() && !o1.is_imm()) {
                    if (o1.is_arg()) {
                        r1 = in.funcs[fidx].func->type->signature->args[o1.arg_idx()].loc;
                    } else {
                        r1 = vmr((u8)vmr::s0 + o1.reg_id());
                        if (t1->is_floating_point) r1 = vmr((u8)vmr::f0 + o1.reg_id());
                    }
                }

                if (o2.is_spilled()) {
                    r2 = vmr::v1;
                    if (t2->is_floating_point) r2 = vmr::vf1;

                    u8 sz = t2->size;
                    if (!t2->is_primitive) sz = sizeof(void*);
                    vmi ld = vmi::ld8;
                    switch (sz) {
                        case 2: { ld = vmi::ld16; break; }
                        case 4: { ld = vmi::ld32; break; }
                        case 8: { ld = vmi::ld64; break; }
                        default: { break; }
                    }
                    m_instructions += encode(ld).operand(r2).operand(vmr::sp).operand((u64)o2.stack_off());
                    m_map.append(i.src);
                } else if (o2.valid() && !o2.is_imm()) {
                    if (o2.is_arg()) {
                        r2 = in.funcs[fidx].func->type->signature->args[o2.arg_idx()].loc;
                    } else {
                        r2 = vmr((u8)vmr::s0 + o2.reg_id());
                        if (t2->is_floating_point) r2 = vmr((u8)vmr::f0 + o2.reg_id());
                    }
                }

                if (o3.is_spilled()) {
                    r3 = vmr::v2;
                    if (t3->is_floating_point) r3 = vmr::vf2;

                    u8 sz = t3->size;
                    if (!o3.type()->is_primitive) sz = sizeof(void*);
                    vmi ld = vmi::ld8;
                    switch (sz) {
                        case 2: { ld = vmi::ld16; break; }
                        case 4: { ld = vmi::ld32; break; }
                        case 8: { ld = vmi::ld64; break; }
                        default: { break; }
                    }
                    m_instructions += encode(ld).operand(r3).operand(vmr::sp).operand((u64)o3.stack_off());
                    m_map.append(i.src);
                } else if (o3.valid() && !o3.is_imm()) {
                    if (o3.is_arg()) {
                        r3 = in.funcs[fidx].func->type->signature->args[o3.arg_idx()].loc;
                    } else {
                        r3 = vmr((u8)vmr::s0 + o3.reg_id());
                        if (t3->is_floating_point) r3 = vmr((u8)vmr::f0 + o3.reg_id());
                    }
                }
            }

            // This comment was written in a police station while waiting for a ride
            switch(i.op) {
                case op::null: {
                    m_instructions += encode(vmi::null);
                    m_map.append(i.src);
                    break;
                }
                case op::term: {
                    m_instructions += encode(vmi::term);
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
                        default: { break; }
                    }
                    // load dest_var imm_addr
                    // load dest_var var_addr
                    // load dest_var var_addr imm_offset

                    if (o2.is_imm()) m_instructions += encode(ld).operand(r1).operand(vmr::zero).operand(o2.imm_u());
                    else {
                        if (o3.is_imm()) m_instructions += encode(ld).operand(r1).operand(r2).operand(o3.imm_u());
                        else m_instructions += encode(ld).operand(r1).operand(r2).operand((u64)0);
                    }
                    
                    m_map.append(i.src);
                    break;
                }
                case op::store: {
                    if (o2.is_imm()) {
                        if (t2->is_floating_point) {
                            r2 = vmr::vf0;
                            if (t2->size == sizeof(f64)) m_instructions += encode(vmi::daddi).operand(r2).operand(vmr::zero).operand(o2.imm_d());
                            else m_instructions += encode(vmi::faddi).operand(r2).operand(vmr::zero).operand(o2.imm_f());
                        } else {
                            r2 = vmr::v0;
                            m_instructions += encode(vmi::addui).operand(r2).operand(vmr::zero).operand(o2.imm_u());
                        }
                        m_map.append(i.src);
                    }
                    
                    u8 sz = t2->size;
                    if (!t2->is_primitive) sz = sizeof(void*);
                    vmi st = vmi::st8;
                    switch (sz) {
                        case 2: { st = vmi::st16; break; }
                        case 4: { st = vmi::st32; break; }
                        case 8: { st = vmi::st64; break; }
                        default: { break; }
                    }

                    m_instructions += encode(st).operand(r2).operand(r1).operand((u64)0);
                    m_map.append(i.src);
                    break;
                }
                case op::stack_alloc: {
                    u64 addr = in.funcs[fidx].stack.alloc(o2.imm_u());
                    if (o1.is_spilled()) {
                        // printf("Allocate[%d][off %d] %d -> 0x%X\n", o1.reg_id(), o1.stack_off(), o2.imm_u(), addr);
                        stack_stack_addrs[o1.stack_off()] = addr;
                        m_instructions += encode(vmi::addui).operand(r1).operand(vmr::sp).operand(addr);
                        m_map.append(i.src);
                        m_instructions += encode(vmi::st64).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                    } else {
                        // printf("Allocate[%d][id %d] %d -> 0x%X\n", o1.reg_id(), o1.stack_id(), o2.imm_u(), addr);
                        reg_stack_addrs[o1.stack_id()] = addr;
                        m_instructions += encode(vmi::addui).operand(r1).operand(vmr::sp).operand(addr);
                    }
                    m_map.append(i.src);
                    break;
                }
                case op::stack_free: {
                    if (o1.is_spilled()) {
                        auto it = stack_stack_addrs.find(o1.stack_off());
                        // printf("Free[%d][off %d] %d\n", o1.reg_id(), o1.stack_off(), it->getSecond());
                        in.funcs[fidx].stack.free(it->getSecond());
                        stack_stack_addrs.erase(it);
                    } else {
                        auto it = reg_stack_addrs.find(o1.stack_id());
                        // printf("Free[%d][id %d] %d\n", o1.reg_id(), o1.stack_id(), it->getSecond());
                        in.funcs[fidx].stack.free(it->getSecond());
                        reg_stack_addrs.erase(it);
                    }
                    break;
                }
                case op::module_data: {
                    m_instructions += encode(vmi::addui).operand(vmr::v3).operand(vmr::zero).operand(o2.imm_u());
                    m_map.append(i.src);
                    m_instructions += encode(vmi::mptr).operand(r1).operand(o3.imm_u());
                    m_map.append(i.src);
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
                    arith(vmi::_and, vmi::andi, vmi::andi);
                    break;
                }
                case op::lor: {
                    arith(vmi::_or, vmi::ori, vmi::ori);
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
                    arith(vmi::_xor, vmi::xori, vmi::xori);
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
                    vmi rr, ri;
                    if (t1->is_floating_point != t2->is_floating_point) {
                        if (o2.is_imm()) {
                            if (t1->is_floating_point) {
                                if (t1->size == sizeof(f64)) {
                                    if (t2->is_unsigned) m_instructions += encode(vmi::daddi).operand(r1).operand(vmr::zero).operand((f64)o2.imm_u());
                                    else m_instructions += encode(vmi::daddi).operand(r1).operand(vmr::zero).operand((f64)o2.imm_i());
                                } else {
                                    if (t2->is_unsigned) m_instructions += encode(vmi::faddi).operand(r1).operand(vmr::zero).operand((f32)o2.imm_u());
                                    else m_instructions += encode(vmi::faddi).operand(r1).operand(vmr::zero).operand((f32)o2.imm_i());
                                }
                            } else {
                                if (t1->is_unsigned) {
                                    if (t2->size == sizeof(f64)) m_instructions += encode(vmi::addui).operand(r1).operand(vmr::zero).operand((u64)o2.imm_d());
                                    else m_instructions += encode(vmi::addui).operand(r1).operand(vmr::zero).operand((u64)o2.imm_f());
                                } else {
                                    if (t2->size == sizeof(f64)) m_instructions += encode(vmi::addi).operand(r1).operand(vmr::zero).operand((i64)o2.imm_d());
                                    else m_instructions += encode(vmi::addi).operand(r1).operand(vmr::zero).operand((i64)o2.imm_f());
                                }
                            }
                        } else {
                            if (t1->is_floating_point) m_instructions += encode(vmi::mtfp).operand(r2).operand(r1);
                            else m_instructions += encode(vmi::mffp).operand(r2).operand(r1);
                        }
                        
                        m_map.append(i.src);
                        break;
                    }

                    if (t1->is_floating_point) {
                        if (t1->size == sizeof(f64)) { rr = vmi::dadd; ri = vmi::daddi; }
                        else { rr = vmi::fadd; ri = vmi::faddi; }
                    } else {
                        if (t1->is_unsigned || !t1->is_primitive) { rr = vmi::addu; ri = vmi::addui; }
                        else { rr = vmi::add; ri = vmi::addi; }
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
                    else m_instructions += encode(rr).operand(r1).operand(r2).operand(vmr::zero);
                    m_map.append(i.src);

                    if (o1.is_spilled()) {
                        u8 sz = t1->size;
                        if (!t1->is_primitive) sz = sizeof(void*);
                        vmi st = vmi::st8;
                        switch (sz) {
                            case 2: { st = vmi::st16; break; }
                            case 4: { st = vmi::st32; break; }
                            case 8: { st = vmi::st64; break; }
                            default: { break; }
                        }
                        m_instructions += encode(st).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                        m_map.append(i.src);
                    }
                    break;
                }
                case op::neg: {
                    if (o2.type()->is_floating_point) {
                        if (o2.size() == sizeof(f64)) m_instructions += encode(vmi::negd).operand(r1).operand(r2);
                        else m_instructions += encode(vmi::negf).operand(r1).operand(r2);
                        m_map.append(i.src);
                    } else if (!o2.type()->is_unsigned) {
                        m_instructions += encode(vmi::neg).operand(r1).operand(r2);
                        m_map.append(i.src);
                    }
                    break;
                }
                case op::call: {
                    function_signature* sig = nullptr;
                    if (i.callee) sig = i.callee->type->signature;
                    else sig = i.callee_v.type()->signature;

                    // If it's a method of a subtype class, pass the subtype ID through $v3
                    if (sig->method_of && sig->method_of->requires_subtype) {
                        // get subtype from the this obj parameter
                        script_type* st = params[0].type()->sub_type;
                        u64 moduletype = join_u32(st->owner->id(), st->id());
                        m_instructions += encode(vmi::addui).operand(vmr::v3).operand(vmr::zero).operand(moduletype);
                        m_map.append(i.src);
                    }
                    
                    struct bk { vmr reg; u64 addr; u64 sz; };
                    std::vector<bk> backup;
                    robin_hood::unordered_map<vm_register, size_t> backup_map;
                    // backup caller's args
                    for (u8 a = 0;a < cf->type->signature->args.size();a++) {
                        vmr ar = in.funcs[fidx].func->type->signature->args[a].loc;

                        auto alt = arg_lifetime_ends.find(ar);
                        if (alt == arg_lifetime_ends.end()) {
                            // arg is totally unused
                            continue;
                        }
                        if (alt->getSecond() < c) {
                            // arg is already unused
                            continue;
                        }

                        bool will_be_overwritten = false;
                        for (u8 a1 = 0;a1 < sig->args.size() && !will_be_overwritten;a1++) {
                            will_be_overwritten = sig->args[a1].loc == ar;
                        }

                        if (!will_be_overwritten) continue;
                        script_type* tp = in.funcs[fidx].func->type->signature->args[a].tp;

                        u8 sz = tp->size;
                        if (!tp->is_primitive) sz = sizeof(void*);
                        vmi st = vmi::st8;
                        switch (sz) {
                            case 2: { st = vmi::st16; break; }
                            case 4: { st = vmi::st32; break; }
                            case 8: { st = vmi::st64; break; }
                            default: { break; }
                        }
                        u64 sl = in.funcs[fidx].stack.alloc(sz);

                        backup.push_back({ ar, sl, sz });
                        backup_map[ar] = backup.size() - 1;

                        m_instructions += encode(st).operand(ar).operand(vmr::sp).operand(sl);
                        m_map.append(i.src);
                    }

                    // if callee's return location is a live register, it needs to be backed up
                    if (sig->return_type->size > 0 && sig->return_loc != vmr::register_count && !vm_reg_is_volatile(sig->return_loc)) {
                        auto b = backup_map.find(sig->return_loc);
                        if (b == backup_map.end()) {
                            // not already backed up
                            auto live = in.funcs[fidx].regs.get_live(c);
                            for (auto& lr : live) {
                                if (lr.end <= c || lr.is_stack()) continue;
                                vmr reg = vmr(lr.is_fp ? u32(vmr::f0) + lr.reg_id : u32(vmr::s0) + lr.reg_id);
                                if (reg == sig->return_loc) {
                                    // callee's return location is currently live, back it up
                                    u64 sl = in.funcs[fidx].stack.alloc(sizeof(u64));
                                    backup.push_back({ reg, sl, sizeof(u64) });
                                    backup_map[reg] = backup.size() - 1;
                                    m_instructions += encode(vmi::st64).operand(reg).operand(vmr::sp).operand(sl);
                                    m_map.append(i.src);
                                    break;
                                }
                            }
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

                                vmr reg;
                                if (params[p1].is_arg()) reg = cf->type->signature->args[params[p1].arg_idx()].loc;
                                else reg = params[p1].type()->is_floating_point ? vmr(u32(vmr::f0) + params[p1].reg_id()) : vmr(u32(vmr::s0) + params[p1].reg_id());
                                will_overwrite = sig->args[p].loc == reg;
                            }

                            if (will_overwrite) continue;
                            passed.push_back(p);

                            // pass the parameter
                            script_type* tp = params[p].type();
                            if (params[p].is_imm()) {
                                if (tp->is_floating_point) {
                                    if (!is_fpr(sig->args[p].loc) && sig->method_of && sig->method_of->requires_subtype) {
                                        // fp value must be passed through GP register
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::daddi).operand(vmr::f15).operand(vmr::zero).operand(params[p].imm_d());
                                        else m_instructions += encode(vmi::faddi).operand(vmr::f15).operand(vmr::zero).operand(params[p].imm_f());
                                        m_map.append(i.src);
                                        m_instructions += encode(vmi::mffp).operand(vmr::f15).operand(sig->args[p].loc);
                                    } else {
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::daddi).operand(sig->args[p].loc).operand(vmr::zero).operand(params[p].imm_d());
                                        else m_instructions += encode(vmi::faddi).operand(sig->args[p].loc).operand(vmr::zero).operand(params[p].imm_f());
                                    }
                                } else {
                                    m_instructions += encode(vmi::addui).operand(sig->args[p].loc).operand(vmr::zero).operand(params[p].imm_u());
                                }
                                m_map.append(i.src);
                            } else if (params[p].is_spilled()) {
                                vmi ld = vmi::ld8;
                                u64 sz = tp->is_primitive ? tp->size : sizeof(void*);
                                switch (sz) {
                                    case 2: { ld = vmi::ld16; break; }
                                    case 4: { ld = vmi::ld32; break; }
                                    case 8: { ld = vmi::ld64; break; }
                                    default: { break; }
                                }
                                m_instructions += encode(ld).operand(sig->args[p].loc).operand(vmr::sp).operand((u64)params[p].stack_off());
                                m_map.append(i.src);
                            } else {
                                vmr reg;
                                if (params[p].is_arg()) {
                                    reg = cf->type->signature->args[params[p].arg_idx()].loc;
                                    auto l = backup_map.find(reg);
                                    if (l != backup_map.end()) {
                                        // argument register was backed up
                                        bk& info = backup[l->getSecond()];

                                        vmi ld = vmi::ld8;
                                        switch (info.sz) {
                                            case 2: { ld = vmi::ld16; break; }
                                            case 4: { ld = vmi::ld32; break; }
                                            case 8: { ld = vmi::ld64; break; }
                                            default: { break; }
                                        }

                                        m_instructions += encode(ld).operand(sig->args[p].loc).operand(vmr::sp).operand((u64)info.addr);
                                        m_map.append(i.src);
                                        continue;
                                    }
                                }
                                else {
                                    if (params[p].is_arg()) reg = cf->type->signature->args[params[p].arg_idx()].loc;
                                    else reg = params[p].type()->is_floating_point ? vmr(u32(vmr::f0) + params[p].reg_id()) : vmr(u32(vmr::s0) + params[p].reg_id());
                                }

                                if (tp->is_floating_point) {
                                    if (!is_fpr(sig->args[p].loc) && sig->method_of && sig->method_of->requires_subtype) {
                                        // fp value must be passed through GP register
                                        m_instructions += encode(vmi::mffp).operand(reg).operand(sig->args[p].loc);
                                    } else {
                                        if (tp->size == sizeof(f64)) m_instructions += encode(vmi::dadd).operand(sig->args[p].loc).operand(vmr::zero).operand(reg);
                                        else m_instructions += encode(vmi::fadd).operand(sig->args[p].loc).operand(vmr::zero).operand(reg);
                                    }
                                } else {
                                    m_instructions += encode(vmi::addu).operand(sig->args[p].loc).operand(vmr::zero).operand(reg);
                                }
                                m_map.append(i.src);
                            }
                        }
                    }

                    params.clear();

                    // do the call
                    if (i.callee) {
                        // offset $sp
                        m_instructions += encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand(in.funcs[fidx].stack.size());
                        m_map.append(i.src);

                        m_instructions += encode(vmi::jal).operand((u64)i.callee->id());
                        m_map.append(i.src);
                    } else {
                        vmr cr = r1;
                        auto l = backup_map.find(cr);
                        if (l != backup_map.end()) {
                            cr = vmr::v0;
                            // function pointer register was backed up
                            bk& info = backup[l->getSecond()];

                            m_instructions += encode(vmi::ld64).operand(cr).operand(vmr::sp).operand((u64)info.addr);
                            m_map.append(i.src);
                        }

                        // offset $sp
                        m_instructions += encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand(in.funcs[fidx].stack.size());
                        m_map.append(i.src);

                        // jalr
                        m_instructions += encode(vmi::jalr).operand(cr);
                        m_map.append(i.src);
                    }

                    // restore $sp
                    m_instructions += encode(vmi::subui).operand(vmr::sp).operand(vmr::sp).operand(in.funcs[fidx].stack.size());
                    m_map.append(i.src);

                    // store return value if necessary
                    if (sig->return_type->size > 0) {
                        vmr ret_reg = i.callee ? r1 : r2;
                        const var& ret_var = i.callee ? o1 : o2;

                        if (ret_var.is_spilled()) {
                            u8 sz = sig->return_type->size;
                            if (!sig->return_type->is_primitive) sz = sizeof(void*);
                            vmi st = vmi::st8;
                            switch (sz) {
                                case 2: { st = vmi::st16; break; }
                                case 4: { st = vmi::st32; break; }
                                case 8: { st = vmi::st64; break; }
                                default: { break; }
                            }
                            m_instructions += encode(st).operand(sig->return_loc).operand(vmr::sp).operand((u64)ret_var.stack_off());
                            m_map.append(i.src);
                        } else {
                            if (sig->returns_pointer) {
                                m_instructions += encode(vmi::addu).operand(ret_reg).operand(sig->return_loc).operand(vmr::zero);
                                m_map.append(i.src);
                            } else if (sig->returns_on_stack) {
                                // return value should have been stored in the implicit
                                // return value pointer passed to the callee
                            } else {
                                if (sig->return_type->is_floating_point) {
                                    if (!is_fpr(sig->return_loc) && sig->method_of && sig->method_of->requires_subtype) {
                                        // fp value returned through gp register
                                        m_instructions += encode(vmi::mtfp).operand(sig->return_loc).operand(ret_reg);
                                    } else {
                                        if (sig->return_type->size == sizeof(f64)) m_instructions += encode(vmi::dadd).operand(ret_reg).operand(vmr::zero).operand(sig->return_loc);
                                        else m_instructions += encode(vmi::fadd).operand(ret_reg).operand(vmr::zero).operand(sig->return_loc);
                                    }
                                } else {
                                    m_instructions += encode(vmi::addu).operand(ret_reg).operand(sig->return_loc).operand(vmr::zero);
                                }
                                m_map.append(i.src);
                            }
                        }
                    }

                    // restore backed up registers
                    for (u16 b = 0;b < backup.size();b++) {
                        vmi ld = vmi::ld8;
                        switch (backup[b].sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: { break; }
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
                    if (o1.valid()) {
                        if (is_fpr(cf->type->signature->return_loc)) m_instructions += encode(vmi::fadd).operand(cf->type->signature->return_loc).operand(r1).operand(vmr::zero);
                        else m_instructions += encode(vmi::addu).operand(cf->type->signature->return_loc).operand(r1).operand(vmr::zero);
                        m_map.append(i.src);
                    }
                    m_instructions += encode(vmi::jmpr).operand(vmr::ra);
                    m_map.append(i.src);
                    break;
                }
                case op::cvt: {
                    // todo: o1 could be an imm...
                    if (o1.is_imm()) {
                        break;
                    }

                    script_type* from = m_ctx->module(extract_left_u32(o2.imm_u()))->types()->get(extract_right_u32(o2.imm_u()));
                    script_type* to = m_ctx->module(extract_left_u32(o3.imm_u()))->types()->get(extract_right_u32(o3.imm_u()));

                    vmi ci = vmi::instruction_count;

                    if (from->is_floating_point) {
                        if (from->size == sizeof(f64)) {
                            if (to->is_floating_point) {
                                if (to->size == sizeof(f32)) ci = vmi::cvt_df;
                            } else {
                                if (to->is_unsigned) ci = vmi::cvt_du;
                                else ci = vmi::cvt_di;
                            }
                        } else {
                            if (to->is_floating_point) {
                                if (to->size == sizeof(f64)) ci = vmi::cvt_fd;
                            } else {
                                if (to->is_unsigned) ci = vmi::cvt_fu;
                                else ci = vmi::cvt_fi;
                            }
                        }
                    } else {
                        if (from->is_unsigned) {
                            if (to->is_floating_point) {
                                if (to->size == sizeof(f64)) ci = vmi::cvt_ud;
                                else ci = vmi::cvt_uf;
                            } else {
                                if (!to->is_unsigned) ci = vmi::cvt_ui;
                            }
                        } else {
                            if (to->is_floating_point) {
                                if (to->size == sizeof(f64)) ci = vmi::cvt_id;
                                else ci = vmi::cvt_if;
                            } else {
                                if (to->is_unsigned) ci = vmi::cvt_iu;
                            }
                        }
                    }

                    if (ci != vmi::instruction_count) {
                        m_instructions += encode(ci).operand(r1);
                        m_map.append(i.src);

                        if (o1.is_spilled()) {
                            u8 sz = t1->size;
                            if (!t1->is_primitive) sz = sizeof(void*);
                            vmi st = vmi::st8;
                            switch (sz) {
                                case 2: { st = vmi::st16; break; }
                                case 4: { st = vmi::st32; break; }
                                case 8: { st = vmi::st64; break; }
                                default: { break; }
                            }
                            m_instructions += encode(st).operand(r1).operand(vmr::sp).operand((u64)o1.stack_off());
                            m_map.append(i.src);
                        }
                    }

                    break;
                }
                case op::label: {
                    label_map[i.labels[0]] = m_instructions.size();
                    break;
                }
                case op::branch: {
                    // todo: Why did I add the b* branch instructions if only bneqz is used
                    if (is_fpr(r1)) {
                        if (t1->size == sizeof(f64)) {
                            m_instructions += encode(vmi::dncmp).operand(vmr::v1).operand(r1).operand(vmr::zero);
                            m_map.append(i.src);
                        } else {
                            m_instructions += encode(vmi::fncmp).operand(vmr::v1).operand(r1).operand(vmr::zero);
                            m_map.append(i.src);
                        }

                        auto lb = label_map.find(i.labels[0]);
                        if (lb != label_map.end()) m_instructions += encode(vmi::bneqz).operand(vmr::v1).operand(lb->second);
                        else {
                            deferred_label_instrs.push_back({ m_instructions.size(), i.labels[0] });
                            m_instructions += encode(vmi::bneqz).operand(vmr::v1);
                        }

                        m_map.append(i.src);
                    } else {
                        auto lb = label_map.find(i.labels[0]);
                        if (lb != label_map.end()) m_instructions += encode(vmi::bneqz).operand(r1).operand(lb->second);
                        else {
                            deferred_label_instrs.push_back({ m_instructions.size(), i.labels[0] });
                            m_instructions += encode(vmi::bneqz).operand(r1);
                        }
                        m_map.append(i.src);
                    }
                    break;
                }
                case op::jump: {
                    if (i.labels[0]) {
                        auto lb = label_map.find(i.labels[0]);
                        if (lb != label_map.end()) m_instructions += encode(vmi::jmp).operand(lb->second);
                        else {
                            deferred_label_instrs.push_back({ m_instructions.size(), i.labels[0] });
                            m_instructions += encode(vmi::jmp);
                        }
                    }
                    else m_instructions += encode(vmi::jmpr).operand(r1);
                    m_map.append(i.src);
                    break;
                }
                //case op::meta_if_branch: break;
                //case op::meta_for_loop: break;
                //case op::meta_while_loop: break;
                //case op::meta_do_while_loop: break;
            }
        }

        // update jumps and branches which referred to labels that were not
        // processed yet at the time of their generation
        for (u32 i = 0;i < deferred_label_instrs.size();i++) {
            instruction& instr = m_instructions[deferred_label_instrs[i].idx];
            auto it = label_map.find(deferred_label_instrs[i].label);
            if (it == label_map.end()) {
                // todo runtime error
                break;
            }

            address addr = it->getSecond();
            instr.operand(addr);
        }

        // all functions should end with jmpr $ra, that final instruction
        // will be removed and replaced with the epilogue, and jmpr $ra will
        // be added back afterward
        address epilog_addr = m_instructions.size() - 1;
        m_instructions.remove(epilog_addr);
        m_map.map.erase(m_map.map.begin() + epilog_addr);

        // replace all jmpr $ra instructions with jmp epilog_addr
        for (u64 c = 0;c < m_instructions.size();c++) {
            auto& i = m_instructions[c];
            if (i.instr() != vmi::jmp || i.op_1r() != vmr::ra) continue;
            m_instructions[c] = encode(vmi::jmp).operand(epilog_addr);
        }

        // generate function epilogue
        for (auto& x : prologue) {
            m_instructions += encode(vmi::ld64).operand(x.reg).operand(vmr::sp).operand(x.addr);
            m_map.append(in.funcs[fidx].ref);
        }

        // add jmpr $ra
        m_instructions += encode(vmi::jmpr).operand(vmr::ra);
        m_map.append(in.funcs[fidx].ref);

        in.funcs[fidx].func->access.entry = out_begin;
    }
    
    u16 vm_backend::gp_count() const {
        return 16;
    }

    u16 vm_backend::fp_count() const {
        return 16;
    }
    
    bool vm_backend::perform_register_allocation() const {
        return true;
    }

    void vm_backend::call(script_function* func, void* ret, void** args) {
        if (func->is_host) {
            func->access.wrapped->call(m_ctx->call_vm(), ret, args);
            return;
        }

        function_signature* sig = func->type->signature;

        for (u8 a = 0;a < sig->args.size();a++) {
            script_type* tp = sig->args[a].tp;
            vm_register loc = sig->args[a].loc;
            u64* dest = &m_vm.state.registers[u8(loc)];

            if (sig->args[a].implicit == function_signature::argument::implicit_type::ret_addr) {
                *dest = (u64)ret;
            } else {
                if (tp->is_primitive) {
                    *dest = (u64)args[a];
                } else {
                    *dest = (u64)args[a];
                }
            }
        }

        execute(func->access.entry);

        if (sig->return_type->size > 0) {
            u64* src = &m_vm.state.registers[u8(sig->return_loc)];
            if (sig->returns_pointer) {
                // todo
                return;
            }

            if (sig->return_type->is_primitive) {
                memcpy(ret, src, sig->return_type->size);
            }
        }
    }
};
