#include <tsn/optimize/ControlFlowGraph.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/optimize/LabelMap.h>
#include <tsn/compiler/IR.h>
#include <tsn/interfaces/IOptimizationStep.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;
        

        basic_block* basic_block::flows_from(u32 idx, ControlFlowGraph* graph) {
            return &graph->blocks[from[idx]];
        }

        basic_block* basic_block::flows_to(u32 idx, ControlFlowGraph* graph) {
            return &graph->blocks[to[idx]];
        }

        bool eventually_flows_to(basic_block* entry, basic_block* target, ControlFlowGraph* graph, robin_hood::unordered_flat_set<basic_block*>& explored) {
            explored.insert(entry);

            for (u8 i = 0;i < entry->to.size();i++) {
                basic_block* to = &graph->blocks[entry->to[i]];
                if (to == target) return true;
                if (explored.count(to)) continue;
                if (eventually_flows_to(to, target, graph, explored)) return true;
            }

            return false;
        }

        bool basic_block::is_loop(ControlFlowGraph* graph) {
            robin_hood::unordered_flat_set<basic_block*> explored;
            return eventually_flows_to(this, this, graph, explored);
        }

        ControlFlowGraph::ControlFlowGraph() {}

        ControlFlowGraph::ControlFlowGraph(compiler::CodeHolder* ch) {
            rebuild(ch);
        }

        void ControlFlowGraph::rebuild(compiler::CodeHolder* ch) {
            blocks.clear();

            if (ch->code.size() == 0) return;

            // generate blocks
            basic_block b = { 0, 0, {}, {} };
            bool push_b = true;
            for (address c = 0;c < ch->code.size();c++) {
                push_b = true;
                const Instruction& i = ch->code[c];
                b.end = c + 1;

                switch (i.op) {
                    case op::ir_label: {
                        if (c == b.begin) break;
                        b.end = c;
                        blocks.push(b);
                        b.begin = c;
                        push_b = false;
                        break;
                    }
                    case op::ir_jump: [[fallthrough]];
                    case op::ir_branch: {
                        blocks.push(b);
                        b.begin = b.end;
                        push_b = false;
                        break;
                    }
                    default: { break; }
                }
            }

            if (push_b) {
                blocks.push(b);
            }

            // generate edges
            for (u32 b = 0;b < blocks.size();b++) {
                basic_block& blk = blocks[b];
                const Instruction& end = ch->code[blk.end - 1];
                switch (end.op) {
                    case op::ir_jump: {
                        // blk jumps directly to some block, one outgoing edge
                        u32 bidx = blockIdxAtAddr(ch->labels.get(end.operands[0].getImm<label_id>()));
                        blocks[bidx].from.push(b);
                        blk.to.push(bidx);
                        break;
                    }
                    case op::ir_branch: {
                        // blk can jump to either of two labels, two outgoing edges
                        u32 bidx;

                        bidx = blockIdxAtAddr(ch->labels.get(end.operands[1].getImm<label_id>()));
                        blocks[bidx].from.push(b);
                        blk.to.push(bidx);

                        bidx = blockIdxAtAddr(ch->labels.get(end.operands[2].getImm<label_id>()));
                        blocks[bidx].from.push(b);
                        blk.to.push(bidx);
                        break;
                    }
                    default: {
                        if (b == blocks.size() - 1) {
                            // end of function
                            break;
                        }

                        // blk falls through to the next block with no jump, one outgoing edge
                        blk.to.push(b + 1);
                        blocks[b + 1].from.push(b);
                        break;
                    }
                }
            }
        }

        basic_block* ControlFlowGraph::blockAtAddr(address a) {
            for (basic_block& b : blocks) {
                if (b.begin == a) return &b;
            }
            return nullptr;
        }

        u32 ControlFlowGraph::blockIdxAtAddr(address a) {
            for (u32 b = 0;b < blocks.size();b++) {
                if (blocks[b].begin == a) return b;
            }
            
            return u32(-1);
        }
    };
};