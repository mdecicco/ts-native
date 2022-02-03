#include <gjs/optimize/optimize.h>
#include <gjs/optimize/cfg.h>
#include <gjs/optimize/liveness.h>
#include <gjs/compiler/variable.h>
#include <gjs/compiler/tac.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>

namespace gjs {
    using namespace compile;
    using op = operation;
    namespace optimize {
        void build_label_map(compilation_output& in, u16 fidx, label_map& lmap) {
            compilation_output::ir_code& code = in.funcs[fidx].code;
            for (u64 i = 0;i < code.size();i++) {
                if (code[i].op == operation::label) lmap[code[i].labels[0]] = i;
            }
        }

        bool vars_match(const var& a, const var& b) {
            script_type* atp = a.type();
            script_type* btp = b.type();
            if (a.is_imm()) {
                if (!b.is_imm()) return false;

                if (atp->is_floating_point) {
                    if (!btp->is_floating_point || atp->size != btp->size) return false;

                    if (atp->size == sizeof(f64)) {
                        if (a.imm_d() != b.imm_d()) return false;
                    } else {
                        if (a.imm_f() != b.imm_f()) return false;
                    }
                } else {
                    if (btp->is_floating_point || atp->is_unsigned != btp->is_unsigned) return false;

                    if (atp->is_unsigned) {
                        if (a.imm_u() != b.imm_u()) return false;
                    } else {
                        if (a.imm_i() != b.imm_i()) return false;
                    }
                }
            } else if (a.is_arg()) {
                if (!b.is_arg() || b.arg_idx() != a.arg_idx()) return false;
            } else {
                if (a.reg_id() != b.reg_id()) return false;
            }

            return true;
        }

        bool constexpr verbose = false;

        void ir_phase_1(script_context* ctx, compilation_output& in, u16 fidx) {
            auto& code = in.funcs[fidx].code;
            if (code.size() == 0) return;

            label_map lmap;
            build_label_map(in, fidx, lmap);

            std::vector<std::pair<u32, u32>> changedAddrs;

            cfg g;
            g.build(in, fidx, lmap);

            liveness l;

            std::vector<tac_instruction*> assignments;
            std::vector<u32> assignmentAddrs;
            u32 changes = 0;
            u32 allChanges = 0;
            for (auto& b : g.blocks) {
                do {
                    allChanges = 0;

                    // copy propagation
                    {
                        changes = 0;
                        robin_hood::unordered_map<u32, var*> assignMap;
                        robin_hood::unordered_map<u32, u32> assignAddrMap;
                        for (address i = b.begin;i < b.end;i++) {
                            if (code[i].op == op::eq) {
                                u32 r = code[i].operands[0].reg_id();
                                if (!code[i].operands[1].is_imm() && !code[i].operands[1].is_arg()) {
                                    auto it = assignMap.find(code[i].operands[1].reg_id());
                                    if (it != assignMap.end()) {
                                        var *v = it->getSecond();
                                        u32 assAddr = assignAddrMap[code[i].operands[1].reg_id()];
                                        if constexpr (verbose) {
                                            printf("\n\npropagating...\n");
                                            printf("[%lu] %s\n...", assAddr, code[assAddr].to_string().c_str());
                                            printf("\n[%llu] %s <- ", i, code[i].to_string().c_str());
                                        }

                                        code[i].operands[1] = *v;
                                        code[i].operands[1].force_cast(code[i].types[1]);
                                        if constexpr (verbose) printf("%s\n", code[i].to_string().c_str());
                                        assignMap[r] = v;
                                        assignAddrMap[r] = (u32)i;
                                        changes++;
                                    } else {
                                        assignMap[r] = &code[i].operands[1];
                                        assignAddrMap[r] = (u32)i;
                                    }
                                } else {
                                    assignMap[r] = &code[i].operands[1];
                                    assignAddrMap[r] = (u32)i;
                                }

                                continue;
                            }

                            const var* assigned = code[i].assignsTo();

                            for (u8 o = 0;o < 3;o++) {
                                var& v = code[i].operands[o];
                                if (!v.valid()) break;
                                if (v.is_arg() || v.is_imm() || v.is_spilled() || (assigned && v.reg_id() == assigned->reg_id())) continue;
                                auto it = assignMap.find(v.reg_id());
                                if (it != assignMap.end()) {
                                    if constexpr (verbose) printf("propagating: [%llu] %s -> ", i, code[i].to_string().c_str());
                                    code[i].operands[o] = *it->second;
                                    code[i].operands[o].force_cast(code[i].types[o]);
                                    if constexpr (verbose) printf("%s\n", code[i].to_string().c_str());
                                }
                            }

                            if (assigned) {
                                // Register assigned to expression result that is evaluated at runtime.
                                // This means that the new value of the register can't be stored in assignMap.
                                // It also means that the register will no longer hold whatever value assignMap
                                // has for the register after this instruction, so that value must be forgotten.
                                assignMap.erase(assigned->reg_id());
                                assignAddrMap.erase(assigned->reg_id());
                            }
                        }
                        allChanges += changes;
                    }

                    // common subexpression elimination
                    {
                        changes = 0;
                        assignments.clear();
                        assignmentAddrs.clear();
                        for (address i = b.begin;i < b.end;i++) {
                            // if the var is assigned externally (no rvalue) then do nothing
                            if (code[i].op == op::cvt || code[i].op == op::stack_alloc || code[i].op == op::call) continue;

                            // If the var is being loaded from an address then do nothing, it's unclear how to
                            // tell if the data at the address being loaded from would be changed in some way.
                            // It must be assumed that two identical load instructions separated by one or more
                            // instruction would not produce the same value in the destination register
                            if (code[i].op == op::load) continue;

                            // if the var was not assigned with a binary expression then do nothing
                            if (code[i].op == op::eq || code[i].op == op::reserve) continue;

                            const var* assigns = code[i].assignsTo();
                            if (!assigns) continue;

                            for (u32 a = 0;a < assignments.size();a++) {
                                tac_instruction& exp = *assignments[a];
                                if (exp.op != code[i].op || exp.operands[0].reg_id() == code[i].operands[0].reg_id()) continue;
                        
                                bool sameArgs = true;
                                for (u8 o = 1;o < 3;o++) {
                                    var& expOp = exp.operands[o];
                                    var& thisOp = code[i].operands[o];
                                    if (!expOp.valid()) break;
                                    if (!vars_match(expOp, thisOp)) {
                                        sameArgs = false;
                                        break;
                                    }
                                }

                                if (sameArgs) {
                                    // if none of the args (including the assignment arg) were assigned between
                                    // now and where _assignment_ is from then code[i] can be replaced with eq
                                    // assigns->reg_id prev.reg_id

                                    bool doUpdate = true;
                                    for (u8 o = 0;o < 3;o++) {
                                        var& expOp = exp.operands[o];
                                        if (!expOp.valid()) break;
                                        if (expOp.is_imm() || expOp.is_arg()) continue;

                                        u32 begin = assignmentAddrs[a];
                                        for (u32 c = begin + 1;c < i;c++) {
                                            const var* assigns = code[c].assignsTo();
                                            if (assigns && assigns->reg_id() == expOp.reg_id()) {
                                                doUpdate = false;
                                                break;
                                            }
                                        }
                                    }
                             
                                    if (doUpdate) {
                                        u32 begin = assignmentAddrs[a];
                                        if constexpr (verbose) printf("\n\neliminating...\n[%lu] %s\n...\n[%llu] %s <- ", begin, exp.to_string().c_str(), i, code[i].to_string().c_str());
                                        const var& prev = exp.operands[0];
                                        code[i].op = operation::eq;
                                        code[i].operands[1] = exp.operands[0];
                                        code[i].operands[2].invalidate();
                                        changes++;
                                        if constexpr (verbose) printf("%s\n", code[i].to_string().c_str());
                                
                                        changedAddrs.push_back({ (u32)i, begin });
                                    }
                                }
                            }

                            assignments.push_back(&code[i]);
                            assignmentAddrs.push_back((u32)i);
                        }
                        allChanges += changes;
                    }
                } while (allChanges > 0);
            }

            if constexpr (verbose) {
                l.build(in, fidx, lmap);

                printf("\n\n\n\n---------------- %s ----------------\n", in.funcs[fidx].func->name.c_str());
                for (u32 b = 0;b < g.blocks.size();b++) {
                    printf("[block %d%s]\n", b, g.blocks[b].is_loop(&g) ? " <loop>" : "");
                    for (address i = g.blocks[b].begin;i < g.blocks[b].end;i++) {
                        bool found = false;
                        for (u32 c = 0;c < changedAddrs.size();c++) {
                            if (changedAddrs[c].first == i) {
                                printf("<%lu>\t[%lld] ", changedAddrs[c].second, i);
                                found = true;
                                break;
                            }
                        }
                        if (!found) printf("\t[%llu] ", i);
                    
                        printf(code[i].to_string().c_str());

                        const var* assigns = code[i].assignsTo();
                        if (assigns) {
                            reg_lifetime* range = l.get_live_range(assigns->reg_id(), i);
                            if (range) printf(" <live until %llu>", range->end);
                        }
                        printf("\n");
                    }
                    printf("Incoming: ");
                    for (u32 i = 0;i < g.blocks[b].from.size();i++) {
                        if (i > 0) printf(", ");
                        printf("%lu", g.blocks[b].from[i]);
                    }
                    printf("\nOutgoing: ");
                    for (u32 i = 0;i < g.blocks[b].to.size();i++) {
                        if (i > 0) printf(", ");
                        printf("%d", g.blocks[b].to[i]);
                    }
                    printf("\n\n\n");
                }
            }
        }

        void dead_code(script_context* ctx, compilation_output& in, u16 fidx) {
            auto& code = in.funcs[fidx].code;
            if (code.size() == 0) return;

            label_map lmap;
            build_label_map(in, fidx, lmap);

            liveness l;
            u32 changes = 0;

            do {
                changes = 0;
                l.build(in, fidx, lmap);

                std::vector<u32> deadAddrs;
                for (auto& r : l.lifetimes) {
                    if (r.usage_count == 0 && code[r.begin].op != op::call) {
                        if constexpr (verbose) printf("dead: [%llu] %s\n", r.begin, code[r.begin].to_string().c_str());
                        deadAddrs.push_back((u32)r.begin);
                    }
                }

                changes = (u32)deadAddrs.size();
                if (changes > 0) {
                    std::sort(deadAddrs.begin(), deadAddrs.end(), [](u32 a, u32 b) { return a > b; });
                    for (u32 addr : deadAddrs) {
                        code.erase(code.begin() + addr);
                    }

                    lmap.clear();
                    build_label_map(in, fidx, lmap);
                }
            } while (changes > 0);
        }
    };
};