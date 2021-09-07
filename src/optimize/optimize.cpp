#include <gjs/optimize/optimize.h>
#include <gjs/compiler/tac.h>
#include <gjs/compiler/variable.h>
#include <gjs/common/pipeline.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>

namespace gjs {
    using namespace compile;
    using op = operation;
    using label_map = robin_hood::unordered_map<label_id, address>;

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

        void cfg::build(compilation_output& in, u16 fidx) {
            compilation_output::ir_code& code = in.funcs[fidx].code;
            if (code.size() == 0) return;

            // build label_id->address map
            label_map lmap;
            for (u64 i = 0;i < code.size();i++) {
                if (code[i].op == operation::label) lmap[code[i].labels[0]] = i;
            }

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

            printf("\n\n\n\n---------------- %s ----------------\n", in.funcs[fidx].func->name.c_str());
            for (u32 b = 0;b < blocks.size();b++) {
                printf("[block %d%s]\n", b, blocks[b].is_loop(this) ? " <loop>" : "");
                for (address i = blocks[b].begin;i < blocks[b].end;i++) {
                    printf("\t%s\n", code[i].to_string().c_str());
                }
                printf("Incoming: ");
                for (u32 i = 0;i < blocks[b].from.size();i++) {
                    if (i > 0) printf(", ");
                    printf("%d", blocks[b].from[i]);
                }
                printf("\nOutgoing: ");
                for (u32 i = 0;i < blocks[b].to.size();i++) {
                    if (i > 0) printf(", ");
                    printf("%d", blocks[b].to[i]);
                }
                printf("\n\n\n");
            }
        }

        void test_step(script_context* ctx, compilation_output& in, u16 fidx) {
            cfg g;
            g.build(in, fidx);
        }
    };
};