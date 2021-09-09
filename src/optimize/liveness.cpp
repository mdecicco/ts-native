#include <gjs/optimize/liveness.h>
#include <gjs/compiler/tac.h>
#include <gjs/compiler/variable.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_type.h>

namespace gjs {
    using namespace compile;

    namespace optimize {
        void liveness::build(compilation_output& in, u16 fidx, label_map& lmap) {
            lifetimes.clear();
            reg_lifetime_map.clear();

            compilation_output::ir_code& code = in.funcs[fidx].code;
            if (code.size() == 0) return;

            for (u64 i = 0;i < code.size();i++) {
                const var* assigns = code[i].assignsTo();
                if (!assigns) continue;

                if (is_live(assigns->m_reg_id, i)) continue;

                reg_lifetime l = { assigns->m_reg_id, i, i, 0, assigns->type()->is_floating_point };

                bool do_calc = true;
                while (do_calc) {
                    for (u64 i1 = l.end + 1;i1 < code.size();i1++) {
                        const var* assigned = code[i1].assignsTo();
                        if (assigned && assigned->m_reg_id == l.reg_id) {
                            if (code[i1].op == operation::cvt || code[i1].involves(l.reg_id, true)) {
                                // if the operation is cvt, the instruction depends on the value of the register,
                                // so the lifetime of register must be extended.
                                // likewise, if the instruction involves the register's value beyond just assigning it,
                                // it also depends on the value of the register.
                                l.usage_count++;
                                l.end = i1;
                                continue;
                            }

                            break;
                        }

                        if (code[i1].involves(l.reg_id)) {
                            l.end = i1;
                            l.usage_count++;
                        }
                    }

                    do_calc = false;
                    for (u64 i1 = l.end + 1;i1 < code.size();i1++) {
                        tac_instruction& instr1 = code[i1];
                        // If a backwards jump goes into a live range,
                        // then that live range must be extended to fit
                        // the jump (if it doesn't already)
                        u64 jaddr = u64(-1);

                        if (instr1.op == operation::jump) jaddr = lmap[instr1.labels[0]];
                        else if (instr1.op == operation::branch) jaddr = lmap[instr1.labels[0]];

                        if (jaddr != u64(-1)) {
                            if (jaddr > i1) continue;
                            if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                l.end = i1;
                                do_calc = true;
                            }
                        }
                    }
                }

                reg_lifetime_map[l.reg_id].push_back(lifetimes.size());
                lifetimes.push_back(l);
            }
        }

        std::vector<reg_lifetime*> liveness::ranges_of(const var& v) {
            if (!v.valid() || v.is_imm()) return {};
            auto it = reg_lifetime_map.find(v.m_reg_id);
            if (it == reg_lifetime_map.end()) return {};

            auto& rangeIndices = it->getSecond();
            std::vector<reg_lifetime*> ranges;
            for (u32 i : rangeIndices) ranges.push_back(&lifetimes[i]);
            return ranges;
        }

        std::vector<reg_lifetime*> liveness::ranges_of(u32 reg_id) {
            auto it = reg_lifetime_map.find(reg_id);
            if (it == reg_lifetime_map.end()) return {};

            auto& rangeIndices = it->getSecond();
            std::vector<reg_lifetime*> ranges;
            for (u32 i : rangeIndices) ranges.push_back(&lifetimes[i]);
            return ranges;
        }

        bool liveness::is_live(const compile::var& v, address at) {
            if (!v.valid() || v.is_imm()) return false;
            auto it = reg_lifetime_map.find(v.m_reg_id);
            if (it == reg_lifetime_map.end()) return false;

            auto& rangeIndices = it->getSecond();
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return true;
            }

            return false;
        }

        bool liveness::is_live(u32 reg_id, address at) {
            auto it = reg_lifetime_map.find(reg_id);
            if (it == reg_lifetime_map.end()) return false;

            auto& rangeIndices = it->getSecond();
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return true;
            }

            return false;
        }

        reg_lifetime* liveness::get_live_range(const compile::var& v, address at) {
            if (!v.valid() || v.is_imm()) return nullptr;
            auto it = reg_lifetime_map.find(v.m_reg_id);
            if (it == reg_lifetime_map.end()) return nullptr;

            auto& rangeIndices = it->getSecond();
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return &range;
            }

            return nullptr;
        }

        reg_lifetime* liveness::get_live_range(u32 reg_id, address at) {
            auto it = reg_lifetime_map.find(reg_id);
            if (it == reg_lifetime_map.end()) return nullptr;

            auto& rangeIndices = it->getSecond();
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return &range;
            }

            return nullptr;
        }
    };
};