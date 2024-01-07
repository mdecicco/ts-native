#include <tsn/optimize/CopyPropagation.h>
#include <tsn/optimize/types.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/optimize/OptimizationGroup.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/Logger.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>

#include <utils/robin_hood.h>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;

        CopyPropagationStep::CopyPropagationStep(Context* ctx) : IOptimizationStep(ctx) {
        }

        CopyPropagationStep::~CopyPropagationStep() {
        }

        bool CopyPropagationStep::execute(compiler::CodeHolder* ch, basic_block* b, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("CopyPropagationStep: Analyzing %d to %d of %s", b->begin, b->end, ch->owner->getFullyQualifiedName().c_str())
                );
            }

            robin_hood::unordered_map<u32, Value*> assignMap;
            robin_hood::unordered_map<u32, u32> assignAddrMap;

            bool hasChanges = false;

            for (address i = b->begin;i < b->end;i++) {
                bool isAdd0 = ch->code[i].op == op::ir_uadd && ch->code[i].operands[2].isImm() && ch->code[i].operands[2].getImm<u64>() == 0;
                isAdd0 = isAdd0 || (ch->code[i].op == op::ir_iadd && ch->code[i].operands[2].isImm() && ch->code[i].operands[2].getImm<i64>() == 0);

                if (ch->code[i].op == op::ir_assign || isAdd0) {
                    u32 r = ch->code[i].operands[0].getRegId();
                    if (!ch->code[i].operands[1].isImm() && !ch->code[i].operands[1].isArg()) {
                        auto it = assignMap.find(ch->code[i].operands[1].getRegId());
                        if (it != assignMap.end()) {
                            Value *v = it->second;
                            address assAddr = assignAddrMap[ch->code[i].operands[1].getRegId()];
                            if (doDebug) {
                                log->submit(lt_debug, lm_generic, "Propagating...");
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("^ (assignment) [%lu] %s...", assAddr, ch->code[assAddr].toString(m_ctx).c_str())
                                );
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("^ (destination) [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                                );
                            }

                            ffi::DataType* tp = ch->code[i].operands[1].getType();
                            ch->code[i].operands[1].reset(*v);
                            ch->code[i].operands[1].setType(tp);
                            hasChanges = true;

                            if (doDebug) {
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("^ (result) [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                                );
                            }

                            assignMap[r] = v;
                            assignAddrMap[r] = (u32)i;
                        } else {
                            assignMap[r] = &ch->code[i].operands[1];
                            assignAddrMap[r] = (u32)i;
                        }
                    } else {
                        assignMap[r] = &ch->code[i].operands[1];
                        assignAddrMap[r] = (u32)i;
                    }

                    continue;
                }

                const Value* assigned = ch->code[i].assigns();

                for (u8 o = 0;o < 3;o++) {
                    Value& v = ch->code[i].operands[o];
                    if (!v.isValid()) break;
                    if (v.isArg() || v.isImm() /* || v.is_spilled()*/ || (assigned && v.getRegId() == assigned->getRegId())) continue;
                    auto it = assignMap.find(v.getRegId());
                    if (it != assignMap.end()) {
                        if (doDebug) {
                            log->submit(lt_debug, lm_generic, "Propagating...");
                            log->submit(
                                lt_debug,
                                lm_generic,
                                utils::String::Format("^ (original) [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                            );
                        }

                        ffi::DataType* tp = ch->code[i].operands[o].getType();
                        ch->code[i].operands[o].reset(*it->second);
                        ch->code[i].operands[o].setType(tp);
                        hasChanges = true;

                        if (doDebug) {
                            log->submit(
                                lt_debug,
                                lm_generic,
                                utils::String::Format("^ (updated) [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                            );
                        }
                    }
                }

                if (assigned) {
                    // Register assigned to expression result that is evaluated at runtime.
                    // This means that the new value of the register can't be stored in assignMap.
                    // It also means that the register will no longer hold whatever value assignMap
                    // has for the register after this instruction, so that value must be forgotten.
                    assignMap.erase(assigned->getRegId());
                    assignAddrMap.erase(assigned->getRegId());
                }
            }

            if (hasChanges) {
                ch->rebuildAll();
                getGroup()->setShouldRepeat(true);
            }

            return hasChanges;
        }
    };
};