#include <gjs/backends/register_allocator.h>
#include <gjs/compiler/tac.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/pipeline.h>

namespace gjs {
    using namespace compile;

    bool register_allocator::reg_lifetime::is_concurrent(const register_allocator::reg_lifetime& o) const {
        return (begin >= o.begin && begin <= o.end) || (o.begin >= begin && o.begin <= end);
    }



    register_allocator::register_allocator(compilation_output& in) : m_in(in), m_fpc(0), m_gpc(0) {
    }

    register_allocator::~register_allocator() {
    }
    
    void register_allocator::process(u16 fidx) {
        if (fidx == u16(-1)) {
            calc_reg_lifetimes(fidx);

            reassign_registers(m_gpLf, m_gpc, fidx);
            reassign_registers(m_fpLf, m_fpc, fidx);

            calc_reg_lifetimes(fidx);
            return;
        }

        compilation_output::func_def& fd = m_in.funcs[fidx];
        calc_reg_lifetimes(fidx);

        reassign_registers(m_gpLf, m_gpc, fidx);
        reassign_registers(m_fpLf, m_fpc, fidx);

        // printf("post-alloc[%s]:\n", fd.func->name.c_str());
        // for (u64 i = 0;i < m_in.funcs[fidx].code.size();i++) printf("%3.3d: %s\n", i, m_in.funcs[fidx].code[i].to_string().c_str());

        calc_reg_lifetimes(fidx);
    }

    std::vector<register_allocator::reg_lifetime> register_allocator::get_live(u64 at) {
        std::vector<register_allocator::reg_lifetime> out;

        for (u16 i = 0;i < m_gpLf.size();i++) {
            if (m_gpLf[i].begin <= at && m_gpLf[i].end >= at) out.push_back(m_gpLf[i]);
        }
        for (u16 i = 0;i < m_fpLf.size();i++) {
            if (m_fpLf[i].begin <= at && m_fpLf[i].end >= at) out.push_back(m_fpLf[i]);
        }

        return out;
    }

    void register_allocator::calc_reg_lifetimes(u16 fidx) {
        m_gpLf.clear();
        m_fpLf.clear();

        robin_hood::unordered_map<label_id, address> label_map;
        compilation_output::ir_code& code = m_in.funcs[fidx].code;
        for (u64 i = 0;i < code.size();i++) {
            if (code[i].op == operation::label) label_map[code[i].labels[0]] = i;
        }


        for (u64 i = 0;i < code.size();i++) {
            tac_instruction& instr0 = code[i];
            bool do_calc = is_assignment(instr0);
            u8 assignedOpIdx = 0;
            if (instr0.op == operation::call && !instr0.callee) assignedOpIdx = 1;
            do_calc = do_calc && instr0.operands[assignedOpIdx].valid();
            do_calc = do_calc && !instr0.operands[assignedOpIdx].is_spilled();
            do_calc = do_calc && !instr0.operands[assignedOpIdx].is_arg();

            if (do_calc) {
                // Determine if this assignment is within an established
                // live range for the same virtual register id
                if (!instr0.operands[assignedOpIdx].type()->is_floating_point) {
                    for (u32 r = 0;r < m_gpLf.size() && do_calc;r++) {
                        if (m_gpLf[r].reg_id != instr0.operands[assignedOpIdx].reg_id()) continue;
                        if (m_gpLf[r].begin <= i && m_gpLf[r].end > i) do_calc = false;
                    }
                } else {
                    for (u32 r = 0;r < m_fpLf.size() && do_calc;r++) {
                        if (m_fpLf[r].reg_id != instr0.operands[assignedOpIdx].reg_id()) continue;
                        if (m_fpLf[r].begin <= i && m_fpLf[r].end > i) do_calc = false;
                    }
                }
            }

            if (!do_calc) continue;

            reg_lifetime l = { instr0.operands[assignedOpIdx].reg_id(), u32(-1), u32(-1), i, i, instr0.operands[assignedOpIdx].type()->is_floating_point };

            while (do_calc) {
                for (u64 i1 = l.end + 1;i1 < code.size();i1++) {
                    const var* assigned = code[i1].assignsTo();
                    if (assigned && assigned->reg_id() == l.reg_id) {
                        if (code[i1].op == operation::cvt || code[i1].involves(l.reg_id, true)) {
                            // if the operation is cvt, the instruction depends on the value of the register,
                            // so the lifetime of register must be continued.
                            // likewise, if the instruction involves the register's value beyond just assigning it,
                            // it also depends on the value of the register.
                            l.end = i1;
                            continue;
                        }
                        
                        break;
                    }

                    if (code[i1].involves(l.reg_id)) {
                        l.end = i1;
                    }
                }

                do_calc = false;
                for (u64 i1 = l.end + 1;i1 < code.size();i1++) {
                    tac_instruction& instr1 = code[i1];
                    // If a backwards jump goes into a live range,
                    // then that live range must be extended to fit
                    // the jump (if it doesn't already)
                    u64 jaddr = u64(-1);

                    if (instr1.op == operation::jump) jaddr = label_map[instr1.labels[0]];
                    else if (instr1.op == operation::branch) jaddr = label_map[instr1.labels[0]];

                    if (jaddr != u64(-1)) {
                        if (jaddr > i1) continue;
                        if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                            l.end = i1;
                            do_calc = true;
                        }
                    }
                }
            }

            if (l.is_fp) m_fpLf.push_back(l);
            else m_gpLf.push_back(l);
        }
    }
    
    void register_allocator::reassign_registers(std::vector<reg_lifetime>& regs, u16 k, u16 fidx) {
        using ti = tac_instruction;
        std::sort(regs.begin(), regs.end(), [](const reg_lifetime& a, const reg_lifetime& b) {
            return a.begin < b.begin;
        });

        func_stack& stack = m_in.funcs[fidx].stack;

        std::vector<u32> free_regs;
        for (u32 r = 0;r < k;r++) free_regs.insert(free_regs.begin(), r);
        std::vector<reg_lifetime*> active;
        std::vector<u32> to_free;
        u32 stack_off = 0;
        for (u32 l = 0;l < regs.size();l++) {
            to_free.clear();
            // Free old regs
            for (u32 o = 0;o < active.size();o++) {
                if (active[o]->end >= regs[l].begin) {
                    break;
                }
                to_free.push_back(o);
                free_regs.push_back(active[o]->new_id);
            }
            
            for (auto f = to_free.rbegin();f != to_free.rend();f++) active.erase(active.begin() + *f);

            if (active.size() == k) {
                reg_lifetime* spill = active.back();
                if (spill->end > regs[l].end) {
                    regs[l].new_id = spill->new_id;
                    script_type* tp = m_in.funcs[fidx].code[spill->begin].operands[0].type();
                    spill->stack_loc = stack.alloc(tp->is_primitive ? tp->size : sizeof(void*));
                    active.pop_back();
                    active.push_back(&regs[l]);
                    std::sort(active.begin(), active.end(), [](reg_lifetime* a, reg_lifetime* b) {
                        return a->end < b->end;
                    });
                }
                else {
                    script_type* tp = m_in.funcs[fidx].code[regs[l].begin].operands[0].type();
                    regs[l].stack_loc = stack.alloc(tp->is_primitive ? tp->size : sizeof(void*));
                }
            } else {
                regs[l].new_id = free_regs.back();
                free_regs.pop_back();
                active.push_back(&regs[l]);
                std::sort(active.begin(), active.end(), [](reg_lifetime* a, reg_lifetime* b) {
                    return a->end < b->end;
                });
            }
        }

        struct change { u64 addr; u8 operand; u32 reg; };
        std::vector<change> changes;

        for (u32 i = 0;i < regs.size();i++) {
            for (u64 c = regs[i].begin;c <= regs[i].end;c++) {
                tac_instruction& instr = m_in.funcs[fidx].code[c];

                for (u8 o = 0;o < 3;o++) {
                    if (instr.operands[o].reg_id() == regs[i].reg_id && instr.operands[o].type()->is_floating_point == regs[i].is_fp) {
                        changes.push_back({ c, o, i });
                    }
                }
                
                if (instr.op == operation::call && !instr.callee && instr.callee_v.reg_id() == regs[i].reg_id && !regs[i].is_fp) {
                    changes.push_back({ c, u8(-1), i });
                    continue;
                }
            }
        }

        for (u32 i = 0;i < changes.size();i++) {
            tac_instruction& instr = m_in.funcs[fidx].code[changes[i].addr];
            if (regs[changes[i].reg].stack_loc == u32(-1)) {
                if (changes[i].operand == u8(-1)) {
                    instr.callee_v.set_register(regs[changes[i].reg].new_id);
                } else {
                    instr.operands[changes[i].operand].set_register(regs[changes[i].reg].new_id);
                }
            } else {
                instr.operands[changes[i].operand].set_stack_loc(regs[changes[i].reg].stack_loc);
            }
        }
    }
};
