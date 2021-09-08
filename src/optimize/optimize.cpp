#include <gjs/optimize/optimize.h>
#include <gjs/optimize/cfg.h>
#include <gjs/optimize/liveness.h>
#include <gjs/compiler/variable.h>
#include <gjs/compiler/tac.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_function.h>

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
        }

        void test_step(script_context* ctx, compilation_output& in, u16 fidx) {
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
                    // common subexpression elimination
                    do {
                        changes = 0;
                        assignments.clear();
                        assignmentAddrs.clear();
                        for (address i = b.begin;i < b.end;i++) {
                            // if the var is assigned externally (no rvalue) then do nothing
                            if (code[i].op == op::cvt || code[i].op == op::stack_alloc || code[i].op == op::call) continue;

                            // if the var was not assigned with a binary expression then do nothing
                            if (code[i].op == op::eq) continue;

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
                                        // printf("\n\neliminating...\n[%d] %s\n...\n[%d] %s <- ", begin, exp.to_string().c_str(), i, code[i].to_string().c_str());
                                        const var& prev = exp.operands[0];
                                        code[i].op = operation::eq;
                                        code[i].operands[1] = exp.operands[0];
                                        code[i].operands[2].invalidate();
                                        changes++;
                                        // printf("%s\n", code[i].to_string().c_str());
                                
                                        changedAddrs.push_back({ i, begin });
                                    }
                                }
                            }

                            assignments.push_back(&code[i]);
                            assignmentAddrs.push_back(i);
                        }
                        allChanges += changes;
                    } while (changes > 0);

                    // copy propagation
                    do {
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
                                        // printf("\n\npropagating...\n");
                                        // printf("[%d] %s\n...", assAddr, code[assAddr].to_string().c_str());
                                        // printf("\n[%d] %s <- ", i, code[i].to_string().c_str());

                                        code[i].operands[1] = *v;
                                        // printf("%s\n", code[i].to_string().c_str());
                                        assignMap[r] = v;
                                        assignAddrMap[r] = i;
                                        changes++;
                                    } else {
                                        assignMap[r] = &code[i].operands[1];
                                        assignAddrMap[r] = i;
                                    }
                                } else {
                                    assignMap[r] = &code[i].operands[1];
                                    assignAddrMap[r] = i;
                                }

                                continue;
                            }

                            const var* assigned = code[i].assignsTo();
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
                    } while (changes > 0);

                    // dead code elimination
                    do {
                        changes = 0;
                        l.build(in, fidx, lmap);
                        std::vector<u32> deadAddrs;
                        for (auto& r : l.lifetimes) {
                            if (r.usage_count == 0) {
                                // printf("dead: [%d] %s\n", r.begin, code[r.begin].to_string().c_str());
                                deadAddrs.push_back(r.begin);
                            }
                        }

                        changes = deadAddrs.size();
                        if (changes > 0) {
                            std::sort(deadAddrs.begin(), deadAddrs.end(), [](u32 a, u32 b) { return a > b; });
                            for (u32 addr : deadAddrs) {
                                for (auto& b1 : g.blocks) {
                                    if (b1.begin >= addr) {
                                        b1.begin--;
                                        b1.end--;
                                    } else if (b1.end >= addr) b1.end--;
                                }
                                code.erase(code.begin() + addr);
                            }

                            lmap.clear();
                            build_label_map(in, fidx, lmap);
                        }

                        allChanges += changes;
                    } while (changes > 0);
                } while (allChanges > 0);
            }
            
            return;
            l.build(in, fidx, lmap);

            printf("\n\n\n\n---------------- %s ----------------\n", in.funcs[fidx].func->name.c_str());
            for (u32 b = 0;b < g.blocks.size();b++) {
                printf("[block %d%s]\n", b, g.blocks[b].is_loop(&g) ? " <loop>" : "");
                for (address i = g.blocks[b].begin;i < g.blocks[b].end;i++) {
                    bool found = false;
                    for (u32 c = 0;c < changedAddrs.size();c++) {
                        if (changedAddrs[c].first == i) {
                            printf("<%d>\t[%d] ", changedAddrs[c].second, i);
                            found = true;
                            break;
                        }
                    }
                    if (!found) printf("\t[%d] ", i);
                    
                    printf(code[i].to_string().c_str());

                    const var* assigns = code[i].assignsTo();
                    if (assigns) {
                        reg_lifetime* range = l.get_live_range(assigns->reg_id(), i);
                        if (range) printf(" <live until %d>", range->end);
                    }
                    printf("\n");
                }
                printf("Incoming: ");
                for (u32 i = 0;i < g.blocks[b].from.size();i++) {
                    if (i > 0) printf(", ");
                    printf("%d", g.blocks[b].from[i]);
                }
                printf("\nOutgoing: ");
                for (u32 i = 0;i < g.blocks[b].to.size();i++) {
                    if (i > 0) printf(", ");
                    printf("%d", g.blocks[b].to[i]);
                }
                printf("\n\n\n");
            }
        }
    };
};