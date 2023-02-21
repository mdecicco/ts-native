#pragma once
#include <tsn/backend/RegisterAllocationStep.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/Logger.h>
#include <tsn/optimize/CodeHolder.h>
#include <tsn/optimize/LabelMap.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>

#include <utils/robin_hood.h>
#include <utils/Array.hpp>

#include <assert.h>

namespace tsn {
    namespace backend {
        using namespace compiler;
        using namespace optimize;
        using op = ir_instruction;

        StackManager::StackManager() {

        }

        StackManager::~StackManager() {

        }

        void StackManager::reset() {
            m_slots.clear();
        }

        u32 StackManager::alloc(u32 sz) {
            // look for unused slot
            for (auto i = m_slots.begin();i != m_slots.end();i++) {
                if (i->in_use) continue;
                u64 s_sz = i->end - i->start;
                if (s_sz > sz) {
                    // split the slot
                    slot n = { i->start + sz, i->end, false };
                    i->end = i->start + sz;
                    i->in_use = true;
                    m_slots.insert(std::next(i), n);
                    return i->start;
                } else if (s_sz == sz) {
                    i->in_use = true;
                    return i->start;
                }
            }

            // create a new slot
            u32 out = (u32)m_slots.size() == 0 ? 0 : m_slots.back().end;
            m_slots.push_back({ out, out + sz, true });
            return out;
        }

        void StackManager::free(u32 addr) {
            for (auto i = m_slots.begin();i != m_slots.end();i++) {
                if (i->start == addr) {
                    i->in_use = false;
                    if (std::next(i) == m_slots.end()) {
                        if (i != m_slots.begin()) {
                            auto p = std::prev(i);
                            if (!p->in_use) {
                                m_slots.erase(i);
                                m_slots.erase(p);
                                return;
                            }
                        }
                        m_slots.erase(i);
                        return;
                    }

                    auto n = std::next(i);
                    if (!n->in_use) {
                        i->end = n->end;
                        m_slots.erase(n);
                    }

                    if (i != m_slots.begin()) {
                        auto p = std::prev(i);
                        if (!p->in_use) {
                            p->end = i->end;
                            m_slots.erase(i);
                        }
                    }

                    return;
                }
            }

            assert(false);
        }

        u32 StackManager::size() const {
            return m_slots.size() == 0 ? 0 : m_slots.back().end;
        }




        bool RegisterAllocatonStep::lifetime::isConcurrent(const RegisterAllocatonStep::lifetime& o) const {
            return (begin >= o.begin && begin <= o.end) || (o.begin >= begin && o.begin <= end);
        }

        bool RegisterAllocatonStep::lifetime::isStack() const {
            return stack_loc == 0xFFFFFFFF;
        }



        RegisterAllocatonStep::RegisterAllocatonStep(Context* ctx, u32 numGP, u32 numFP) : IOptimizationStep(ctx) {
            m_numGP = numGP;
            m_numFP = numFP;
        }

        RegisterAllocatonStep::~RegisterAllocatonStep() {
        }

        bool RegisterAllocatonStep::execute(CodeHolder* ch, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("RegisterAllocatonStep: Analyzing %s", ch->owner->getFullyQualifiedName().c_str())
                );
            }

            m_stack.reset();
            
            m_ch = ch;
            calcLifetimes();

            bool hasChanges = false;
            if (allocateRegisters(m_gpLf, m_numGP, pipeline)) hasChanges = true;
            if (allocateRegisters(m_fpLf, m_numFP, pipeline)) hasChanges = true;

            calcLifetimes();

            return false;
        }

        void RegisterAllocatonStep::getLive(u32 at, utils::Array<RegisterAllocatonStep::lifetime>& live) {
            for (u16 i = 0;i < m_gpLf.size();i++) {
                if (m_gpLf[i].begin <= at && m_gpLf[i].end >= at) live.push(m_gpLf[i]);
            }

            for (u16 i = 0;i < m_fpLf.size();i++) {
                if (m_fpLf[i].begin <= at && m_fpLf[i].end >= at) live.push(m_fpLf[i]);
            }
        }

        void RegisterAllocatonStep::calcLifetimes() {
            m_gpLf.clear();
            m_fpLf.clear();

            auto& code = m_ch->code;
            for (u32 i = 0;i < code.size();i++) {
                Instruction& instr0 = code[i];
                auto& info0 = instruction_info(instr0.op);
                const Value* assigns0 = instr0.assigns();
                bool do_calc = assigns0 && assigns0->isValid() && !assigns0->isStack() && !assigns0->isArg();

                u8 assignedOpIdx = 0;
                if (do_calc) {
                    // Determine if this assignment is within an established
                    // live range for the same virtual register id
                    if (!assigns0->getType()->getInfo().is_floating_point) {
                        for (u32 r = 0;r < m_gpLf.size() && do_calc;r++) {
                            if (m_gpLf[r].reg_id != assigns0->getRegId()) continue;
                            if (m_gpLf[r].begin <= i && m_gpLf[r].end > i) do_calc = false;
                        }
                    } else {
                        for (u32 r = 0;r < m_fpLf.size() && do_calc;r++) {
                            if (m_fpLf[r].reg_id != assigns0->getRegId()) continue;
                            if (m_fpLf[r].begin <= i && m_fpLf[r].end > i) do_calc = false;
                        }
                    }
                }

                if (!do_calc) continue;

                lifetime l = {
                    assigns0->getRegId(),
                    u32(-1),
                    u32(-1),
                    i,
                    i,
                    assigns0->getType()->getInfo().is_floating_point == 1
                };

                while (do_calc) {
                    for (u32 i1 = l.end + 1;i1 < code.size();i1++) {
                        const Value* assigned = code[i1].assigns();
                        if (assigned && assigned->getRegId() == l.reg_id) {
                            if (code[i1].involves(l.reg_id, true)) {
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
                    for (u32 i1 = l.end + 1;i1 < code.size();i1++) {
                        Instruction& instr1 = code[i1];
                        // If a backwards jump goes into a live range,
                        // then that live range must be extended to fit
                        // the jump (if it doesn't already)

                        if (instr1.op == op::ir_jump) {
                            u32 jaddr = m_ch->labels.get(instr1.operands[0].getImm<label_id>());

                            if (jaddr != u32(-1)) {
                                if (jaddr > i1) continue;
                                if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                    l.end = i1;
                                    do_calc = true;
                                }
                            }
                        } else if (instr1.op == op::ir_branch) {
                            u32 jaddr;
                            jaddr = m_ch->labels.get(instr1.operands[1].getImm<label_id>());
                            if (jaddr != u32(-1)) {
                                if (jaddr > i1) continue;
                                if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                    l.end = i1;
                                    do_calc = true;
                                }
                            }

                            jaddr = m_ch->labels.get(instr1.operands[2].getImm<label_id>());
                            if (jaddr != u32(-1)) {
                                if (jaddr > i1) continue;
                                if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                    l.end = i1;
                                    do_calc = true;
                                }
                            }
                        }
                    }
                }

                if (l.is_fp) m_fpLf.push(l);
                else m_gpLf.push(l);
            }
        }

        bool RegisterAllocatonStep::allocateRegisters(utils::Array<RegisterAllocatonStep::lifetime>& live, u16 k, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            std::sort(live.begin(), live.end(), [](const lifetime& a, const lifetime& b) {
                return a.begin < b.begin;
            });

            std::vector<u32> free_regs;
            for (u32 r = 0;r < k;r++) free_regs.insert(free_regs.begin(), r + 1);
            std::vector<lifetime*> active;
            std::vector<u32> to_free;
            u32 stack_off = 0;
            for (u32 l = 0;l < live.size();l++) {
                to_free.clear();
                // Free old regs
                for (u32 o = 0;o < active.size();o++) {
                    if (active[o]->end >= live[l].begin) {
                        break;
                    }
                    to_free.push_back(o);
                    free_regs.push_back(active[o]->new_id);
                }
                
                for (auto f = to_free.rbegin();f != to_free.rend();f++) active.erase(active.begin() + *f);

                if (active.size() == k) {
                    lifetime* spill = active.back();
                    if (spill->end > live[l].end) {
                        live[l].new_id = spill->new_id;
                        ffi::DataType* tp = m_ch->code[spill->begin].operands[0].getType();
                        const auto& ti = tp->getInfo();
                        spill->stack_loc = m_stack.alloc(ti.is_primitive ? ti.size : sizeof(void*));
                        active.pop_back();
                        active.push_back(&live[l]);
                        std::sort(active.begin(), active.end(), [](lifetime* a, lifetime* b) {
                            return a->end < b->end;
                        });
                    }
                    else {
                        ffi::DataType* tp = m_ch->code[live[l].begin].operands[0].getType();
                        const auto& ti = tp->getInfo();
                        live[l].stack_loc = m_stack.alloc(ti.is_primitive ? ti.size : sizeof(void*));
                    }
                } else {
                    live[l].new_id = free_regs.back();
                    free_regs.pop_back();
                    active.push_back(&live[l]);
                    std::sort(active.begin(), active.end(), [](lifetime* a, lifetime* b) {
                        return a->end < b->end;
                    });
                }
            }

            struct change { u32 addr; u8 operand; vreg_id reg; };
            utils::Array<change> changes;

            for (u32 i = 0;i < live.size();i++) {
                for (u32 c = live[i].begin;c <= live[i].end;c++) {
                    Instruction& instr = m_ch->code[c];

                    for (u8 o = 0;o < instr.oCnt;o++) {
                        if (instr.operands[o].getRegId() == live[i].reg_id && instr.operands[o].getType()->getInfo().is_floating_point == live[i].is_fp) {
                            changes.push({ c, o, i });
                        }
                    }
                }
            }

            for (u32 i = 0;i < changes.size();i++) {
                Instruction& instr = m_ch->code[changes[i].addr];
                if (live[changes[i].reg].stack_loc == u32(-1)) {
                    Value before = instr.operands[changes[i].operand];
                    instr.operands[changes[i].operand].setRegId(live[changes[i].reg].new_id);
                    if (doDebug) {
                        log->submit(
                            lt_debug,
                            lm_generic,
                            utils::String::Format(
                                "Reallocate: [%lu] %s | %s -> %s",
                                changes[i].addr,
                                instr.toString(m_ctx).c_str(),
                                before.toString().c_str(),
                                instr.operands[changes[i].operand].toString().c_str()
                            )
                        );
                    }
                } else {
                    Value before = instr.operands[changes[i].operand];
                    instr.operands[changes[i].operand].setStackAllocId(live[changes[i].reg].stack_loc);
                    if (doDebug) {
                        log->submit(
                            lt_debug,
                            lm_generic,
                            utils::String::Format(
                                "Reallocate: [%lu] %s | %s -> %s",
                                changes[i].addr,
                                instr.toString(m_ctx).c_str(),
                                before.toString().c_str(),
                                instr.operands[changes[i].operand].toString().c_str()
                            )
                        );
                    }
                }
            }

            return changes.size() > 0;
        }
    };
};