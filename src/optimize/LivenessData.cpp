#include <tsn/optimize/LivenessData.h>
#include <tsn/optimize/LabelMap.h>
#include <tsn/optimize/CodeHolder.h>
#include <tsn/compiler/Value.h>
#include <tsn/compiler/IR.h>
#include <tsn/ffi/DataType.h>
#include <tsn/interfaces/IOptimizationStep.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;

        LivenessData::LivenessData() {}
        
        LivenessData::LivenessData(CodeHolder* ch) {
            rebuild(ch);
        }
        
        void LivenessData::rebuild(CodeHolder* ch) {
            lifetimes.clear();
            regLifetimeMap.clear();

            if (ch->code.size() == 0) return;

            for (address i = 0;i < ch->code.size();i++) {
                const Value* assigns = ch->code[i].assigns();
                if (!assigns || assigns->isStack()) continue;

                if (isLive(assigns->getRegId(), i)) continue;

                reg_lifetime l = { assigns->getRegId(), i, i, 0, assigns->getType()->getInfo().is_floating_point == 1 };

                bool do_calc = true;
                while (do_calc) {
                    for (address i1 = l.end + 1;i1 < ch->code.size();i1++) {
                        const Value* assigned = ch->code[i1].assigns();
                        if (assigned && assigned->getRegId() == l.reg_id) {
                            if (ch->code[i1].involves(l.reg_id, true)) {
                                // if the instruction involves the register's value beyond just assigning it,
                                // it also depends on the value of the register.
                                l.usage_count++;
                                l.end = i1;
                                continue;
                            }

                            break;
                        }

                        if (ch->code[i1].involves(l.reg_id)) {
                            l.end = i1;
                            l.usage_count++;
                        }
                    }

                    do_calc = false;
                    for (address i1 = l.end + 1;i1 < ch->code.size();i1++) {
                        const Instruction& instr1 = ch->code[i1];
                        // If a backwards jump goes into a live range,
                        // then that live range must be extended to fit
                        // the jump (if it doesn't already)

                        if (instr1.op == op::ir_jump) {
                            // one jump address
                            address jaddr = ch->labels.get(instr1.operands[0].getImm<label_id>());
                            if (jaddr > i1) continue;
                            if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                l.end = i1;
                                do_calc = true;
                            }
                        } else if (instr1.op == op::ir_branch) {
                            // two jump addresses
                            address jaddr;
                            
                            jaddr = ch->labels.get(instr1.operands[1].getImm<label_id>());
                            if (jaddr > i1) continue;
                            if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                l.end = i1;
                                do_calc = true;
                            }
                            
                            jaddr = ch->labels.get(instr1.operands[2].getImm<label_id>());
                            if (jaddr > i1) continue;
                            if (l.begin < jaddr && l.end >= jaddr && l.end < i1) {
                                l.end = i1;
                                do_calc = true;
                            }
                        }
                    }
                }

                regLifetimeMap[l.reg_id].push(lifetimes.size());
                lifetimes.push(l);
            }
        }

        utils::Array<reg_lifetime*> LivenessData::rangesOf(const Value& v) {
            if (!v.isValid() || v.isImm()) return {};
            auto it = regLifetimeMap.find(v.getRegId());
            if (it == regLifetimeMap.end()) return {};

            auto& rangeIndices = it->second;
            utils::Array<reg_lifetime*> ranges;
            for (u32 i : rangeIndices) ranges.push(&lifetimes[i]);
            return ranges;
        }

        utils::Array<reg_lifetime*> LivenessData::rangesOf(u32 reg_id) {
            auto it = regLifetimeMap.find(reg_id);
            if (it == regLifetimeMap.end()) return {};

            auto& rangeIndices = it->second;
            utils::Array<reg_lifetime*> ranges;
            for (u32 i : rangeIndices) ranges.push(&lifetimes[i]);
            return ranges;
        }

        bool LivenessData::isLive(const compiler::Value& v, address at) {
            if (!v.isValid() || v.isImm()) return false;
            auto it = regLifetimeMap.find(v.getRegId());
            if (it == regLifetimeMap.end()) return false;

            auto& rangeIndices = it->second;
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return true;
            }

            return false;
        }

        bool LivenessData::isLive(u32 reg_id, address at) {
            auto it = regLifetimeMap.find(reg_id);
            if (it == regLifetimeMap.end()) return false;

            auto& rangeIndices = it->second;
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return true;
            }

            return false;
        }

        reg_lifetime* LivenessData::getLiveRange(const compiler::Value& v, address at) {
            if (!v.isValid() || v.isImm()) return nullptr;
            auto it = regLifetimeMap.find(v.getRegId());
            if (it == regLifetimeMap.end()) return nullptr;

            auto& rangeIndices = it->second;
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return &range;
            }

            return nullptr;
        }

        reg_lifetime* LivenessData::getLiveRange(u32 reg_id, address at) {
            auto it = regLifetimeMap.find(reg_id);
            if (it == regLifetimeMap.end()) return nullptr;

            auto& rangeIndices = it->second;
            for (u32 i : rangeIndices) {
                auto& range = lifetimes[i];
                if (range.begin <= at && range.end > at) return &range;
            }

            return nullptr;
        }
    };
};