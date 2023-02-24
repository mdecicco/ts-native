#include <tsn/optimize/ReduceMemoryAccessStep.h>
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

        ReduceMemoryAccessStep::ReduceMemoryAccessStep(Context* ctx) : IOptimizationStep(ctx) {
        }

        ReduceMemoryAccessStep::~ReduceMemoryAccessStep() {
        }

        bool ReduceMemoryAccessStep::execute(compiler::CodeHolder* ch, Pipeline* pipeline) {
            // TODO:
            // if function is not pure then ONLY look for and optimize these patterns:
            //
            // [n    ] store $GPXX $GPYY
            // [n + 1] load  $GPZZ $GPYY <- will be eliminated by dead code eliminator
            // ...
            // [n + x] (non assignment) $GPZZ... <- $GPZZ should be replaced with $GPXX
            //
            // -------------
            // (I haven't seen this pattern yet but it could be optimized without consequence for non-pure functions)
            // [n    ] load  $GPXX $GPYY
            // [n + 1] store $GPXX $GPYY <-- should be eliminated
            //

            //
            // Post-implementation note:
            // Something to consider:
            //     Two different registers might hold the same pointer
            //     in such a case, it could be possible for a store operation
            //     for just one of them to not be picked up by load operations
            //     for the other one... Thus betraying the intent of the written
            //     code.
            //
            // Possible solution:
            //     Track reads/writes from/to known pointer values independently
            //     of what register they happen to be stored in
            //
            // Requires:
            //     Knowing where all pointers are located at the very start of the
            //     function.
            //

            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("ReduceMemoryAccessStep: Analyzing %s", ch->owner->getFullyQualifiedName().c_str())
                );
            }

            struct load_info {
                address loadedAt;
                Value* loadedFrom;
                Value* loadedTo;
                bool wasOverwritten;
            };
            struct store_info {
                address storedAt;
                Value* sourceValue;
                Value* destValue;
            };

            robin_hood::unordered_map<vreg_id, load_info> loadMap;
            robin_hood::unordered_map<vreg_id, store_info> storeMap;

            // this will hold registers that have not been assigned since being loaded to
            robin_hood::unordered_map<vreg_id, address> lastAssign;

            utils::Array<address> removeAddrs;
            bool hasChanges = false;

            for (address c = 0;c < ch->code.size();c++) {
                Instruction& i = ch->code[c];
                if (i.op == ir_load) {
                    // Possibilities:
                    // 1. This is a fresh load (no previous stores/loads to/from this address)
                    //    ^ Just save the load/assignment information for later
                    // 2. Another register already has the value loaded from this address
                    //    ^ Patch instruction to ir_assign [to] [prev_loaded_value]
                    //    ^ Blocked by: prev register reassigned since being loaded
                    //                  value at address overwritten, or side effects occurred
                    // 3. Another register already has the value that was stored in this address
                    //    ^ Patch instruction to ir_assign [to] [prev_stored_value]
                    //    ^ Blocked by: prev value is register and was reassigned since being stored
                    //                  value at address overwritten, or side effects occurred
                    // 4. The register that is receiving the loaded value was already used to load the value
                    //    ^ Remove the instruction
                    //    ^ Blocked by: prev register reassigned since being loaded
                    //                  value at address overwritten, or side effects occurred
                    // 5. The register that is receiving the loaded value was already stored in the address
                    //    ^ Remove the instruction
                    //    ^ Blocked by: prev register reassigned since being loaded
                    //                  value at address overwritten, or side effects occurred
                    // 6. One of 2-5 happened, but was blocked due to some condition
                    //    ^ Just save the load/assignment information for later

                    vreg_id from = i.operands[1].getRegId();
                    vreg_id to = i.operands[0].getRegId();
                    auto prevStore = storeMap.find(from);
                    auto prevLoad = loadMap.find(from);
                    bool wasHandled = false;

                    if (prevLoad == loadMap.end() && prevStore == storeMap.end()) {
                        // Scenario #1
                        loadMap[from] = {
                            // loaded at, loaded from, loaded to, wasOverwritten
                            c, &i.operands[1], &i.operands[0], false
                        };
                        lastAssign[to] = c;
                        continue;
                    }

                    if (prevLoad != loadMap.end()) {
                        // Possibly #2, #3, #4, #5
                        load_info& pl = prevLoad->second;

                        if (prevStore != storeMap.end()) {
                            store_info& ps = prevStore->second;
                            // hint: ps can only be the _most recent_ store op for this address,
                            //       ruling out overwriting as a possibility. And side effects
                            //       are handled by clearing all the stored info, meaning that
                            //       if that happend since the store we wouldn't even _be_ here.

                            if (ps.storedAt > pl.loadedAt) {
                                // Possibly #3, #5

                                if (ps.sourceValue->isReg()) {
                                    if (ps.sourceValue->getRegId() == to) {
                                        // Possibly #5

                                        auto la = lastAssign.find(to);
                                        if (la == lastAssign.end() || la->second < ps.storedAt) {
                                            // Scenario #5
                                            removeAddrs.push(c);
                                            wasHandled = true;
                                            hasChanges = true;
                                            if (doDebug) {
                                                log->submit(
                                                    lt_debug,
                                                    lm_generic,
                                                    utils::String::Format("[%lu] %s <- Unnecessary load (load destination unmodified since being stored in address)", c, i.toString(m_ctx).c_str())
                                                );
                                            }
                                        }
                                    } else {
                                        // Possibly #3
                                        auto la = lastAssign.find(ps.sourceValue->getRegId());
                                        if (la == lastAssign.end() || la->second < ps.storedAt) {
                                            if (doDebug) {
                                                log->submit(
                                                    lt_debug,
                                                    lm_generic,
                                                    utils::String::Format("[%lu] %s <- Unnecessary load (another register already contains the value that would be loaded)", c, i.toString(m_ctx).c_str())
                                                );
                                            }
                                            
                                            // Scenario #3
                                            i.op = ir_assign;
                                            i.operands[1].reset(*ps.sourceValue);
                                            i.oCnt = 2;

                                            if (doDebug) {
                                                log->submit(
                                                    lt_debug,
                                                    lm_generic,
                                                    utils::String::Format("^ [%lu] %s (updated)", c, i.toString(m_ctx).c_str())
                                                );
                                            }

                                            lastAssign[i.operands[0].getRegId()] = c;
                                            wasHandled = true;
                                            hasChanges = true;
                                        }
                                    }
                                } else {
                                    // Scenario #3 (non-register)
                                    // If imm: can't be reassigned, and other blockers irrelevant (see hint)
                                    // If stack: Too complicated for my tiny rat brain to comprehend right now.
                                    //           Just ignore that case until the solution becomes clear. (Ignoring
                                    //           it won't produce bad code, but doing the wrong thing might.)

                                    if (ps.sourceValue->isImm()) {
                                        if (doDebug) {
                                            log->submit(
                                                lt_debug,
                                                lm_generic,
                                                utils::String::Format("[%lu] %s <- Unnecessary load (most recent stored value was a constant)", c, i.toString(m_ctx).c_str())
                                            );
                                        }

                                        i.op = ir_assign;
                                        i.operands[1].reset(*ps.sourceValue);
                                        i.oCnt = 2;

                                        if (doDebug) {
                                            log->submit(
                                                lt_debug,
                                                lm_generic,
                                                utils::String::Format("^ [%lu] %s (updated)", c, i.toString(m_ctx).c_str())
                                            );
                                        }

                                        lastAssign[i.operands[0].getRegId()] = c;
                                        hasChanges = true;
                                    }
                                    
                                    wasHandled = true;
                                }
                            }

                            if (wasHandled) continue;
                        }

                        // Possibly #2, #4
                        if (pl.loadedTo->getRegId() == to) {
                            // Possibly #4

                            auto la = lastAssign.find(to);
                            if (la == lastAssign.end() || la->second <= pl.loadedAt) {
                                if (!pl.wasOverwritten) {
                                    // Scenario #4
                                    removeAddrs.push(c);
                                    wasHandled = true;
                                    hasChanges = true;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary load (load destination unmodified since receiving the value loaded from the same unmodified source)", c, i.toString(m_ctx).c_str())
                                        );
                                    }
                                }
                            }
                        } else {
                            // Possibly #2
                            auto la = lastAssign.find(pl.loadedTo->getRegId());
                            if (la == lastAssign.end() || la->second < pl.loadedAt) {
                                if (!pl.wasOverwritten) {
                                    // Scenario #2
                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary load (another register already contains the value that would be loaded)", c, i.toString(m_ctx).c_str())
                                        );
                                    }

                                    i.op = ir_assign;
                                    i.operands[1].reset(*pl.loadedTo);
                                    i.oCnt = 2;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("^ [%lu] %s (updated)", c, i.toString(m_ctx).c_str())
                                        );
                                    }

                                    lastAssign[i.operands[0].getRegId()] = c;
                                    wasHandled = true;
                                    hasChanges = true;
                                }
                            }
                        }
                    } else {
                        store_info& ps = prevStore->second;
                        // Possibly #3, #5
                        // hint: ps can only be the _most recent_ store op for this address,
                        //       ruling out overwriting as a possibility. And side effects
                        //       are handled by clearing all the stored info, meaning that
                        //       if that happend since the store we wouldn't even _be_ here.
                        if (ps.sourceValue->isReg()) {
                            if (ps.sourceValue->getRegId() == to) {
                                // Possibly #5

                                auto la = lastAssign.find(to);
                                if (la == lastAssign.end() || la->second < ps.storedAt) {
                                    // Scenario #5
                                    removeAddrs.push(c);
                                    wasHandled = true;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary load (load destination unmodified since being stored in address)", c, i.toString(m_ctx).c_str())
                                        );
                                    }
                                }
                            } else {
                                // Possibly #3
                                auto la = lastAssign.find(ps.sourceValue->getRegId());
                                if (la == lastAssign.end() || la->second < ps.storedAt) {
                                    // Scenario #3
                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary load (another register contains value that would be loaded)", c, i.toString(m_ctx).c_str())
                                        );
                                    }

                                    i.op = ir_assign;
                                    i.operands[1].reset(*ps.sourceValue);
                                    i.oCnt = 2;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("^ [%lu] %s (updated)", c, i.toString(m_ctx).c_str())
                                        );
                                    }

                                    lastAssign[i.operands[0].getRegId()] = c;
                                    wasHandled = true;
                                }
                            }
                        } else {
                            // Scenario #3 (non-register)
                            // If imm: can't be reassigned, and other blockers irrelevant (see hint)
                            // If stack: Too complicated for my tiny rat brain to comprehend right now.
                            //           Just ignore that case until the solution becomes clear. (Ignoring
                            //           it won't produce bad code, but doing the wrong thing might.)

                            if (ps.sourceValue->isImm()) {
                                if (doDebug) {
                                    log->submit(
                                        lt_debug,
                                        lm_generic,
                                        utils::String::Format("[%lu] %s <- Unnecessary load (most recent stored value was a constant)", c, i.toString(m_ctx).c_str())
                                    );
                                }

                                i.op = ir_assign;
                                i.operands[1].reset(*ps.sourceValue);
                                i.oCnt = 2;

                                if (doDebug) {
                                    log->submit(
                                        lt_debug,
                                        lm_generic,
                                        utils::String::Format("^ [%lu] %s (updated)", c, i.toString(m_ctx).c_str())
                                    );
                                }

                                lastAssign[i.operands[0].getRegId()] = c;
                            }
                            
                            wasHandled = true;
                        }
                    }

                    if (!wasHandled) {
                        loadMap[from] = {
                            // loaded at, loaded from, loaded to, wasOverwritten
                            c, &i.operands[1], &i.operands[0], false
                        };
                        lastAssign[to] = c;
                    }
                } else if (i.op == ir_store) {
                    // Possibilities: (keep in mind, previous store data is the _most recent_ store data)
                    // 1. This is a fresh store (no previous store/load instructions wrote/read from this address)
                    //     ^ Just save the store information
                    // 2. This is storing a value which was previously loaded from the address
                    //     ^ Remove the instruction
                    //     ^ Blocked by: side effects, prev loaded value reassigned, value at address changed since load
                    // 3. This is storing a register which was previously stored in the address
                    //     ^ Remove the instruction
                    //     ^ Blocked by: side effects, previous register reassigned since last store
                    // 4. This is storing an imm in the address which was previously stored in the address
                    //     ^ Remove the instruction
                    //     ^ Blocked by: side effects
                    // 5. This is storing a stack value in the address which was previously stored in the address
                    //     ^ ??? ignore for now
                    //     ^ Blocked by: ???

                    vreg_id at = i.operands[1].getRegId();
                    bool wasHandled = false;
                    auto prevStore = storeMap.find(at);
                    auto prevLoad = loadMap.find(at);
                    if (prevLoad == loadMap.end() && prevStore == storeMap.end()) {
                        // Scenario #1
                        storeMap[at] = {
                            c, &i.operands[0], &i.operands[1]
                        };
                        continue;
                    }

                    if (prevLoad != loadMap.end()) {
                        auto& pl = prevLoad->second;

                        // #2, #3, #4, #5
                        if (prevStore != storeMap.end()) {
                            auto& ps = prevStore->second;

                            if (ps.storedAt > pl.loadedAt) {
                                // #3, #4, #5
                                if (ps.sourceValue->isReg()) {
                                    if (ps.sourceValue->isEquivalentTo(i.operands[0])) {
                                        // #3
                                        auto la = lastAssign.find(ps.sourceValue->getRegId());
                                        if (la == lastAssign.end() || la->second <= pl.loadedAt) {
                                            // yup
                                            removeAddrs.push(c);
                                            wasHandled = true;
                                            hasChanges = true;

                                            if (doDebug) {
                                                log->submit(
                                                    lt_debug,
                                                    lm_generic,
                                                    utils::String::Format("[%lu] %s <- Unnecessary store (source/dest unmodified since last identical store)", c, i.toString(m_ctx).c_str())
                                                );
                                            }
                                        }
                                    }
                                } else if (ps.sourceValue->isImm()) {
                                    // #4
                                    if (i.operands[0].isEquivalentTo(*ps.sourceValue)) {
                                        // yup
                                        removeAddrs.push(c);
                                        wasHandled = true;
                                        hasChanges = true;

                                        if (doDebug) {
                                            log->submit(
                                                lt_debug,
                                                lm_generic,
                                                utils::String::Format("[%lu] %s <- Unnecessary store (dest unmodified since last identical store, and source is constant)", c, i.toString(m_ctx).c_str())
                                            );
                                        }
                                    }
                                } else {
                                    // #5
                                    // do nothing
                                }
                            } else {
                                // #2
                                if (pl.loadedTo->isEquivalentTo(i.operands[0])) {
                                    auto la = lastAssign.find(pl.loadedTo->getRegId());
                                    if (la == lastAssign.end() || la->second <= pl.loadedAt) {
                                        // yup
                                        removeAddrs.push(c);
                                        wasHandled = true;
                                        hasChanges = true;

                                        if (doDebug) {
                                            log->submit(
                                                lt_debug,
                                                lm_generic,
                                                utils::String::Format("[%lu] %s <- Unnecessary store (source was loaded from dest, and both are unmodified since then)", c, i.toString(m_ctx).c_str())
                                            );
                                        }
                                    }
                                }
                            }
                        } else {
                            // #2
                            if (pl.loadedTo->isEquivalentTo(i.operands[0])) {
                                auto la = lastAssign.find(pl.loadedTo->getRegId());
                                if (la == lastAssign.end() || la->second <= pl.loadedAt) {
                                    // yup
                                    removeAddrs.push(c);
                                    wasHandled = true;
                                    hasChanges = true;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary store (source was loaded from dest, and both are unmodified since then)", c, i.toString(m_ctx).c_str())
                                        );
                                    }
                                }
                            }
                        }
                    } else {
                        // #3, #4, #5
                        auto& ps = prevStore->second;
                        // #3, #4, #5
                        if (ps.sourceValue->isReg()) {
                            if (ps.sourceValue->isEquivalentTo(i.operands[0])) {
                                // #3
                                auto la = lastAssign.find(ps.sourceValue->getRegId());
                                if (la == lastAssign.end() || la->second < ps.storedAt) {
                                    // yup
                                    removeAddrs.push(c);
                                    wasHandled = true;
                                    hasChanges = true;

                                    if (doDebug) {
                                        log->submit(
                                            lt_debug,
                                            lm_generic,
                                            utils::String::Format("[%lu] %s <- Unnecessary store (source/dest unmodified since last identical store)", c, i.toString(m_ctx).c_str())
                                        );
                                    }
                                }
                            }
                        } else if (ps.sourceValue->isImm()) {
                            // #4
                            if (i.operands[0].isEquivalentTo(*ps.sourceValue)) {
                                // yup
                                removeAddrs.push(c);
                                wasHandled = true;
                                hasChanges = true;

                                if (doDebug) {
                                    log->submit(
                                        lt_debug,
                                        lm_generic,
                                        utils::String::Format("[%lu] %s <- Unnecessary store (dest unmodified since last identical store, and source is constant)", c, i.toString(m_ctx).c_str())
                                    );
                                }
                            }
                        } else {
                            // #5
                            // do nothing
                        }
                    }

                    if (!wasHandled) {
                        storeMap[at] = {
                            c, &i.operands[0], &i.operands[1]
                        };

                        if (prevLoad != loadMap.end()) {
                            prevLoad->second.wasOverwritten = true;
                        }
                    }
                } else {
                    auto& info = instruction_info(i.op);
                    if (info.has_side_effects) {
                        loadMap.clear();
                        storeMap.clear();
                    } else {
                        const Value* assigns = i.assigns();
                        if (assigns) lastAssign[assigns->getRegId()] = c;
                    }
                }
            }

            if (removeAddrs.size() > 0) {
                std::sort(
                    removeAddrs.begin(),
                    removeAddrs.end(),
                    [](address a, address b) {
                        return a > b;
                    }
                );

                for (address addr : removeAddrs) {
                    ch->code.remove(addr);
                }

                ch->rebuildAll();
            }

            if (hasChanges) getGroup()->setShouldRepeat(true);

            return false;
        }
    };
};