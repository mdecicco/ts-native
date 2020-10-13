#include <backends/register_allocator.h>
#include <compiler/tac.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/pipeline.h>

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
            calc_reg_lifetimes(fidx, 0, m_in.code.size() - 1);

            reassign_registers(m_gpLf, m_gpc, 0, m_in.code.size() - 1, fidx);
            reassign_registers(m_fpLf, m_fpc, 0, m_in.code.size() - 1, fidx);

            calc_reg_lifetimes(fidx, 0, m_in.code.size() - 1);
            return;
        }

        compilation_output::func_def& fd = m_in.funcs[fidx];
        calc_reg_lifetimes(fidx, fd.begin, fd.end);

        reassign_registers(m_gpLf, m_gpc, fd.begin, fd.end, fidx);
        reassign_registers(m_fpLf, m_fpc, fd.begin, fd.end, fidx);

        // printf("post-alloc[%s]:\n", fd.func->name.c_str());
        // for (u64 i = fd.begin;i <= fd.end;i++) printf("%3.3d: %s\n", i, m_in.code[i].to_string().c_str());

        calc_reg_lifetimes(fidx, fd.begin, fd.end);
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

    void register_allocator::calc_reg_lifetimes(u16 fidx, u64 from, u64 to) {
        m_gpLf.clear();
        m_fpLf.clear();

        for (u64 i = from;i <= to;i++) {
            if (fidx == 0) {
                // ignore all code within functions other than __init__
                bool ignore = false;
                for (u16 f = 1;f < m_in.funcs.size() && !ignore;f++) ignore = i >= m_in.funcs[f].begin && i <= m_in.funcs[f].end;
                if (ignore) continue;
            }

            bool do_calc = is_assignment(m_in.code[i]);
            do_calc = do_calc && m_in.code[i].operands[0].valid();
            do_calc = do_calc && !m_in.code[i].operands[0].is_spilled();
            do_calc = do_calc && !m_in.code[i].operands[0].is_arg();

            if (do_calc) {
                // Determine if this assignment is within an established
                // live range for the same virtual register id
                if (!m_in.code[i].operands[0].type()->is_floating_point) {
                    for (u32 r = 0;r < m_gpLf.size() && do_calc;r++) {
                        if (m_gpLf[r].reg_id != m_in.code[i].operands[0].m_reg_id) continue;
                        if (i < m_gpLf[r].end) do_calc = false;
                    }
                } else {
                    for (u32 r = 0;r < m_fpLf.size() && do_calc;r++) {
                        if (m_fpLf[r].reg_id != m_in.code[i].operands[0].m_reg_id) continue;
                        if (i < m_fpLf[r].end) do_calc = false;
                    }
                }
            }

            if (!do_calc) continue;

            reg_lifetime l = { m_in.code[i].operands[0].m_reg_id, u32(-1), u32(-1), i, i, m_in.code[i].operands[0].type()->is_floating_point };
            for (u64 i1 = i + 1;i1 <= to;i1++) {
                if (fidx == 0) {
                    // ignore all code within functions other than __init__
                    bool ignore = false;
                    for (u16 f = 1;f < m_in.funcs.size() && !ignore;f++) ignore = i1 >= m_in.funcs[f].begin && i1 <= m_in.funcs[f].end;
                    if (ignore) continue;
                }

                u8 o = 0;
                for (;o < 3;o++) {
                    if (m_in.code[i1].operands[o].m_reg_id == l.reg_id && m_in.code[i1].operands[o].type()->is_floating_point == l.is_fp) break;
                }

                if (o == 3) continue;
                if (o > 0 || !is_assignment(m_in.code[i1])) l.end = i1;
                else if (o == 0 && is_assignment(m_in.code[i1])) break;
            }
            for (u64 i1 = i + 1;i1 <= to;i1++) {
                if (fidx == 0) {
                    // ignore all code within functions other than __init__
                    bool ignore = false;
                    for (u16 f = 1;f < m_in.funcs.size() && !ignore;f++) ignore = i1 >= m_in.funcs[f].begin && i1 <= m_in.funcs[f].end;
                    if (ignore) continue;
                }
                // If a backwards jump goes into a live range,
                // then that live range must be extended to fit
                // the jump (if it doesn't already)
                u64 jaddr = u64(-1);

                if (m_in.code[i1].op == operation::jump) jaddr = m_in.code[i1].operands[0].imm_u();
                else if (m_in.code[i].op == operation::branch) jaddr = m_in.code[i1].operands[1].imm_u();

                if (jaddr != u64(-1)) {
                    if (jaddr > i1) continue;
                    if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                        l.end = i1;
                    }
                }
            }

            if (l.is_fp) m_fpLf.push_back(l);
            else m_gpLf.push_back(l);
        }
    }
    
    void register_allocator::reassign_registers(std::vector<reg_lifetime>& regs, u16 k, u64 from, u64 to, u16 fidx) {
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
                    script_type* tp = m_in.code[spill->begin].operands[0].type();
                    spill->stack_loc = stack.alloc(tp->is_primitive ? tp->size : sizeof(void*));
                    active.pop_back();
                    active.push_back(&regs[l]);
                    std::sort(active.begin(), active.end(), [](reg_lifetime* a, reg_lifetime* b) {
                        return a->end < b->end;
                    });
                }
                else {
                    script_type* tp = m_in.code[regs[l].begin].operands[0].type();
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
                if (fidx == 0) {
                    // ignore all code within functions other than __init__
                    bool ignore = false;
                    for (u16 f = 1;f < m_in.funcs.size() && !ignore;f++) ignore = c >= m_in.funcs[f].begin && c <= m_in.funcs[f].end;
                    if (ignore) continue;
                }

                for (u8 o = 0;o < 3;o++) {
                    if (m_in.code[c].operands[o].m_reg_id == regs[i].reg_id) {
                        changes.push_back({ c, o, i });
                    }
                }
            }
        }

        for (u32 i = 0;i < changes.size();i++) {
            if (regs[changes[i].reg].stack_loc == u32(-1)) {
                m_in.code[changes[i].addr].operands[changes[i].operand].m_reg_id = regs[changes[i].reg].new_id;
            } else {
                m_in.code[changes[i].addr].operands[changes[i].operand].m_stack_loc = regs[changes[i].reg].stack_loc;
            }
        }
    }
};