#include <tsn/optimize/CommonSubexpressionElimination.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/optimize/OptimizationGroup.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/Logger.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>
#include <tsn/pipeline/Pipeline.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;

        CommonSubexpressionEliminationStep::CommonSubexpressionEliminationStep(Context* ctx) : IOptimizationStep(ctx) {
        }

        CommonSubexpressionEliminationStep::~CommonSubexpressionEliminationStep() {
        }

        bool CommonSubexpressionEliminationStep::execute(compiler::CodeHolder* ch, basic_block* b, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("CommonSubexpressionEliminationStep: Analyzing %d to %d of %s", b->begin, b->end, ch->owner->getFullyQualifiedName().c_str())
                );
            }

            bool hasChanges = false;
            utils::Array<Instruction*> assignments;
            utils::Array<address> assignmentAddrs;
            for (address i = b->begin;i < b->end;i++) {
                // If the var is being loaded from an address then do nothing...
                // Todo: if no instructions with side effects occur between two identical load instructions,
                //       they produce the same result
                if (ch->code[i].op == op::ir_load) continue;

                // if the var was not assigned with a binary expression then do nothing
                if (ch->code[i].op == op::ir_assign || ch->code[i].op == op::ir_reserve) continue;

                const Value* assigns = ch->code[i].assigns();
                if (!assigns) continue;

                for (u32 a = 0;a < assignments.size();a++) {
                    const Instruction& expr = *assignments[a];
                    const auto& einfo = instruction_info(expr.op);
                    u8 aidx = einfo.assigns_operand_index;
                    if (expr.op != ch->code[i].op || expr.operands[aidx].isEquivalentTo(ch->code[i].operands[aidx])) continue;
            
                    bool sameArgs = true;
                    for (u8 o = 0;o < expr.oCnt;o++) {
                        if (o == aidx) continue;
                        const Value& expOp = expr.operands[o];
                        const Value& thisOp = ch->code[i].operands[o];
                        if (expOp.isValid() != thisOp.isValid() || !expOp.isEquivalentTo(thisOp)) {
                            sameArgs = false;
                            break;
                        }
                    }

                    if (sameArgs) {
                        // if none of the args (including the assignment arg) were assigned between
                        // now and where _assignment_ is from then code[i] can be replaced with
                        // ir_assig assigns prev

                        bool doUpdate = true;
                        for (u8 o = 0;o < expr.oCnt;o++) {
                            const Value& expOp = expr.operands[o];
                            if (!expOp.isValid()) break;
                            if (expOp.isImm()) continue;

                            u32 begin = assignmentAddrs[a];
                            for (u32 c = begin + 1;c < i;c++) {
                                const Value* assigns = ch->code[c].assigns();
                                if (assigns && assigns->getRegId() == expOp.getRegId()) {
                                    doUpdate = false;
                                    break;
                                }
                            }
                        }
                    
                        if (doUpdate) {
                            u32 begin = assignmentAddrs[a];
                            if (doDebug) {
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("Eliminating [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                                );
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("^ Previously [%lu] %s", begin, expr.toString(m_ctx).c_str())
                                );
                            }

                            const Value& prev = expr.operands[0];
                            ch->code[i].op = op::ir_assign;
                            ch->code[i].operands[1].reset(expr.operands[0]);
                            ch->code[i].operands[2].reset(Value());
                            ch->code[i].oCnt = 2;

                            if (doDebug) {
                                log->submit(
                                    lt_debug,
                                    lm_generic,
                                    utils::String::Format("^ Updated to [%lu] %s", i, ch->code[i].toString(m_ctx).c_str())
                                );
                            }
                    
                            hasChanges = true;
                        }
                    }
                }

                assignments.push(&ch->code[i]);
                assignmentAddrs.push(i);
            }
            
            if (hasChanges) {
                ch->rebuildAll();
                getGroup()->setShouldRepeat(true);
            }

            return hasChanges;
        }
    };
};