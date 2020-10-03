#include <backends/register_allocator.h>
#include <compiler/tac.h>
#include <common/script_type.h>
#include <common/pipeline.h>

namespace gjs {
    using namespace compile;

    bool register_allocator::reg_lifetime::is_concurrent(const register_allocator::reg_lifetime& o) const {
        return (begin >= o.begin && begin <= o.end) || (o.begin >= begin && o.begin <= end);
    }



    register_allocator::register_allocator(compilation_output& in) : m_in(in) {
    }

    register_allocator::~register_allocator() {
    }
    
    void register_allocator::process(u16 fidx) {
        compilation_output::func_def& fd = m_in.funcs[fidx];
        // printf("Pre-allocation\n");
        calc_reg_lifetimes(fd.begin, fd.end);
        /*
        for (u16 i = 0;i < m_gpLf.size();i++) {
            printf("$R%d: %d -> %d\n", m_gpLf[i].reg_id, m_gpLf[i].begin, m_gpLf[i].end);
        }
        for (u16 i = 0;i < m_fpLf.size();i++) {
            printf("$F%d: %d -> %d\n", m_fpLf[i].reg_id, m_fpLf[i].begin, m_fpLf[i].end);
        }
        */

        // std::vector<tac_instruction> old = m_in.code;

        reassign_registers(m_gpLf, m_gpc, fd.begin, fd.end, fidx);
        reassign_registers(m_fpLf, m_fpc, fd.begin, fd.end, fidx);

        // printf("Post-allocation\n");
        calc_reg_lifetimes(fd.begin, fd.end);
        /*
        for (u16 i = 0;i < m_gpLf.size();i++) {
            printf("$R%d: %d -> %d\n", m_gpLf[i].reg_id, m_gpLf[i].begin, m_gpLf[i].end);
        }
        for (u16 i = 0;i < m_fpLf.size();i++) {
            printf("$F%d: %d -> %d\n", m_fpLf[i].reg_id, m_fpLf[i].begin, m_fpLf[i].end);
        }
        */

        //printf("New code\n");
        /*
        for (u32 i = fd.begin;i <= fd.end;i++) {
            printf("%3.3d: %-35s %s\n", i, m_in.code[i].to_string().c_str(), old[i].to_string().c_str());
        }
        */
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

    void register_allocator::calc_reg_lifetimes(u64 from, u64 to) {
        m_gpLf.clear();
        m_fpLf.clear();
        m_nextStackIdx = 0;

        for (u64 i = from;i <= to;i++) {
            // If a backwards jump goes into a live range,
            // then that live range must be extended to fit
            // the jump (if it doesn't already)
            if (m_in.code[i].op == operation::jump) {
                u64 jaddr = m_in.code[i].operands[0].imm_u();
                if (jaddr > i) continue;
                for (u32 r = 0;r < m_gpLf.size();r++) {
                    if (m_gpLf[r].begin <= jaddr && m_gpLf[r].end >= jaddr && m_gpLf[r].end < i) {
                        m_gpLf[r].end = i;
                    }
                }
                for (u32 r = 0;r < m_fpLf.size();r++) {
                    if (m_fpLf[r].begin <= jaddr && m_fpLf[r].end >= jaddr && m_fpLf[r].end < i) {
                        m_fpLf[r].end = i;
                    }
                }
            } else if (m_in.code[i].op == operation::branch) {
                u64 jaddr = m_in.code[i].operands[1].imm_u();
                if (jaddr > i) continue;
                for (u32 r = 0;r < m_gpLf.size();r++) {
                    if (m_gpLf[r].begin <= jaddr && m_gpLf[r].end >= jaddr && m_gpLf[r].end < i) {
                        m_gpLf[r].end = i;
                    }
                }
                for (u32 r = 0;r < m_fpLf.size();r++) {
                    if (m_fpLf[r].begin <= jaddr && m_fpLf[r].end >= jaddr && m_fpLf[r].end < i) {
                        m_fpLf[r].end = i;
                    }
                }
            }

            bool needs_reassignment = is_assignment(m_in.code[i].op);
            needs_reassignment = needs_reassignment && m_in.code[i].operands[0].valid();
            needs_reassignment = needs_reassignment && !m_in.code[i].operands[0].is_spilled();
            needs_reassignment = needs_reassignment && !m_in.code[i].operands[0].is_arg();


            if (!needs_reassignment) continue;

            reg_lifetime l = { m_in.code[i].operands[0].m_reg_id, -1, -1, i, i, m_in.code[i].operands[0].type()->is_floating_point };
            for (u64 i1 = i + 1;i1 <= to;i1++) {
                u8 o = 0;
                for (;o < 3;o++) {
                    if (m_in.code[i1].operands[o].m_reg_id == l.reg_id && m_in.code[i1].operands[o].type()->is_floating_point == l.is_fp) break;
                }

                if (o == 3) continue;
                if (o > 0 || !is_assignment(m_in.code[i1].op)) l.end = i1;
                else if (o == 0 && is_assignment(m_in.code[i1].op)) break;
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
                    spill->stack_loc = m_in.funcs[fidx].stack.alloc(tp->is_primitive ? tp->size : sizeof(void*));
                    active.pop_back();
                    active.push_back(&regs[l]);
                    std::sort(active.begin(), active.end(), [](reg_lifetime* a, reg_lifetime* b) {
                        return a->end < b->end;
                    });
                }
                else {
                    script_type* tp = m_in.code[regs[l].begin].operands[0].type();
                    regs[l].stack_loc = m_in.funcs[fidx].stack.alloc(tp->is_primitive ? tp->size : sizeof(void*));
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

        for (u32 i = 0;i < regs.size();i++) {
            for (u32 c = regs[i].begin;c <= regs[i].end;c++) {
                for (u32 o = 0;o < 3;o++) {
                    if (m_in.code[c].operands[o].m_reg_id == regs[i].reg_id) {
                        if (regs[i].stack_loc == u32(-1)) {
                            m_in.code[c].operands[o].m_reg_id = regs[i].new_id;
                        } else {
                            m_in.code[c].operands[o].m_stack_loc = regs[i].stack_loc;
                        }
                    }
                }
            }
        }
    }
};