#include <tsn/optimize/DeadCodeElimination.h>
#include <tsn/optimize/LabelMap.h>
#include <tsn/optimize/LivenessData.h>
#include <tsn/optimize/CodeHolder.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.h>
#include <tsn/compiler/Logger.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;

        DeadCodeEliminationStep::DeadCodeEliminationStep(Context* ctx) : IOptimizationStep(ctx) {
        }

        DeadCodeEliminationStep::~DeadCodeEliminationStep() {
        }

        bool DeadCodeEliminationStep::execute(CodeHolder* ch, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("DeadCodeEliminationStep: Analyzing %s", ch->owner->getFullyQualifiedName().c_str())
                );
            }

            utils::Array<u32> deadAddrs;
            for (auto& r : ch->liveness.lifetimes) {
                if (r.usage_count == 0 && ch->code[r.begin].op != op::ir_call) {
                    if (doDebug) {
                        log->submit(
                            lt_debug,
                            lm_generic,
                            utils::String::Format("dead: [%llu] %s\n", r.begin, ch->code[r.begin].toString(m_ctx).c_str())
                        );
                    }

                    deadAddrs.push((u32)r.begin);
                }
            }

            if (deadAddrs.size() > 0) {
                std::sort(
                    deadAddrs.begin(),
                    deadAddrs.end(),
                    [](u32 a, u32 b) {
                        return a > b;
                    }
                );

                for (u32 addr : deadAddrs) {
                    ch->code.remove(addr);
                }

                ch->rebuildAll();
                 
                return true;
            }

            return false;
        }
    };
};