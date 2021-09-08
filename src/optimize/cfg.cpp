#include <gjs/optimize/cfg.h>
#include <gjs/optimize/optimize.h>
#include <gjs/compiler/tac.h>
#include <gjs/compiler/variable.h>
#include <gjs/common/pipeline.h>

namespace gjs {
    using namespace compile;
    using op = operation;

    namespace optimize {
        basic_block* basic_block::flows_from(u8 idx, cfg* graph) {
            return &graph->blocks[from[idx]];
        }

        basic_block* basic_block::flows_to(u8 idx, cfg* graph) {
            return &graph->blocks[to[idx]];
        }

        bool eventually_flows_to(basic_block* entry, basic_block* target, cfg* graph, robin_hood::unordered_flat_set<basic_block*>& explored) {
            explored.insert(entry);

            for (u8 i = 0;i < entry->to.size();i++) {
                basic_block* to = &graph->blocks[entry->to[i]];
                if (to == target) return true;
                if (explored.count(to)) continue;
                if (eventually_flows_to(to, target, graph, explored)) return true;
            }

            return false;
        }

        bool basic_block::is_loop(cfg* graph) {
            robin_hood::unordered_flat_set<basic_block*> explored;
            return eventually_flows_to(this, this, graph, explored);
        }

        basic_block* cfg::blockAtAddr(address a) {
            for (basic_block& b : blocks) {
                if (b.begin == a) return &b;
            }
            return nullptr;
        }

        u32 cfg::blockIdxAtAddr(address a) {
            for (u32 b = 0;b < blocks.size();b++) {
                if (blocks[b].begin == a) return b;
            }
            return u32(-1);
        }

        void cfg::build(compilation_output& in, u16 fidx, label_map& lmap) {
            blocks.clear();

            compilation_output::ir_code& code = in.funcs[fidx].code;
            if (code.size() == 0) return;

            // generate blocks
            basic_block b = { 0, 0, {}, {} };
            bool push_b = true;
            for (address c = 0;c < code.size();c++) {
                push_b = true;
                const tac_instruction& i = code[c];
                b.end = c + 1;

                switch (i.op) {
                    case op::label: {
                        if (c == b.begin) break;
                        b.end = c;
                        blocks.push_back(b);
                        b.begin = c;
                        push_b = false;
                        break;
                    }
                    case op::jump: [[fallthrough]];
                    case op::branch: {
                        blocks.push_back(b);
                        b.begin = b.end;
                        push_b = false;
                        break;
                    }
                    default: { break; }
                }
            }

            if (push_b) {
                blocks.push_back(b);
            }

            // generate edges
            for (u32 b = 0;b < blocks.size();b++) {
                basic_block& blk = blocks[b];
                tac_instruction& end = code[blk.end - 1];
                switch (end.op) {
                    case op::jump: {
                        // blk jumps directly to some block, one outgoing edge
                        u32 bidx = blockIdxAtAddr(lmap[end.labels[0]]);
                        blocks[bidx].from.push_back(b);
                        blk.to.push_back(bidx);
                        break;
                    }
                    case op::branch: {
                        // blk can fall through to the next block without jumping AND can jump to some block, two outgoing edges
                        blk.to.push_back(b + 1);
                        blocks[b + 1].from.push_back(b);

                        u32 bidx = blockIdxAtAddr(lmap[end.labels[0]]);
                        blocks[bidx].from.push_back(b);
                        blk.to.push_back(bidx);
                        break;
                    }
                    default: {
                        if (b == blocks.size() - 1) {
                            // end of function
                            break;
                        }

                        // blk falls through to the next block with no jump, one outgoing edge
                        blk.to.push_back(b + 1);
                        blocks[b + 1].from.push_back(b);
                        break;
                    }
                }
            }
        }
    };
};