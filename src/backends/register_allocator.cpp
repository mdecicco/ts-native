#include <backends/register_allocator.h>
#include <compiler/tac.h>
#include <common/script_type.h>
#include <common/pipeline.h>

namespace gjs {
    using namespace compile;

    register_allocator::register_allocator(compilation_output& in, u16 gpN, u16 fpN) : m_gpc(gpN), m_fpc(fpN), m_in(in) {
    }

    register_allocator::~register_allocator() {
    }

    void register_allocator::process() {
        for (u16 i = 0;i < m_in.funcs.size();i++) process_func(i);
    }

    bool register_allocator::reg_lifetime::is_concurrent(const register_allocator::reg_lifetime& o) const {
        return (begin >= o.begin && begin <= o.end) || (o.begin >= begin && o.begin <= end);
    }

    void register_allocator::process_func(u16 fidx) {
        compilation_output::func_def& fd = m_in.funcs[fidx];
        calc_reg_lifetimes(fd.begin, fd.end);
        for (u16 i = 0;i < m_gpLf.size();i++) {
            printf("$R%d: %d -> %d\n", m_gpLf[i].reg_id, m_gpLf[i].begin, m_gpLf[i].end);
        }
        for (u16 i = 0;i < m_fpLf.size();i++) {
            printf("$F%d: %d -> %d\n", m_fpLf[i].reg_id, m_fpLf[i].begin, m_fpLf[i].end);
        }

        if (!requires_spill(m_gpLf, m_gpc)) {
            // perform register id reassignment operation
        } else {
            // determine which GPR to push to stack and when, then
            // insert the appropriate instructions to do so
        }

        if (!requires_spill(m_fpLf, m_fpc)) {
            // perform register id reassignment operation
        } else {
            // determine which GPR to push to stack and when, then
            // insert the appropriate instructions to do so
        }
    }

    void register_allocator::calc_reg_lifetimes(u64 from, u64 to) {
        m_gpLf.clear();
        m_fpLf.clear();
        auto calced = [this](u32 reg_id) {
            for (u32 i = 0;i < m_gpLf.size();i++) {
                if (m_gpLf[i].reg_id == reg_id) return true;
            }
            for (u32 i = 0;i < m_fpLf.size();i++) {
                if (m_fpLf[i].reg_id == reg_id) return true;
            }
            return false;
        };
        for (u64 i = from;i <= to;i++) {
            for (u8 o = 0;o < 3;o++) {
                var& v = m_in.code[i].operands[o];
                if (!v.valid() || v.is_imm()) continue;

                if (!calced(v.reg_id())) {
                    reg_lifetime l = { v.reg_id(), i, i };

                    for (u64 i1 = i;i1 <= to;i1++) {
                        for (u8 o1 = 0;o1 < 3;o1++) {
                            var& v1 = m_in.code[i1].operands[o1];
                            if (v1.reg_id() == l.reg_id) l.end = i1;
                        }
                    }

                    if (!v.type()->is_floating_point || m_fpc == 0) m_gpLf.push_back(l);
                    else m_fpLf.push_back(l);
                }
            }
        }
    }
    
    bool register_allocator::requires_spill(const std::vector<reg_lifetime>& regs, u16 k) {
        struct node {
            reg_lifetime l;

            // neighbors, registers that partially or completely share the same lifetime
            std::vector<node*> c;
        };
        std::vector<node*> graph;

        for (u32 i = 0;i < regs.size();i++) {
            node* n = new node();
            n->l = regs[i];
            graph.push_back(n);
        }

        for (u32 i = 0;i < graph.size();i++) {
            node* n = graph[i];

            for (u32 i1 = 0;i1 < graph.size();i1++) {
                if (i1 == i) continue;
                if (n->l.is_concurrent(graph[i1]->l)) {
                    n->c.push_back(graph[i1]);
                }
            }
        }

        bool is_colorable = false;
        std::vector<node*> removed;
        while (!is_colorable) {
            node* remove = nullptr;
            for (u32 i = 0;i < graph.size() && !remove;i++) {
                node* n = graph[i];
                if (n->c.size() >= k) continue;

                remove = n;
            }

            if (!remove) {
                break;
            }

            for (u32 i = 0;i < graph.size();i++) {
                node* n = graph[i];
                for (u16 i1 = 0;i1 < n->c.size();i1++) {
                    if (n->c[i1] == remove) {
                        n->c.erase(n->c.begin() + i1);
                        break;
                    }
                }
            }
            for (u32 i = 0;i < graph.size();i++) {
                if (graph[i] == remove) {
                    graph.erase(graph.begin() + i);
                    break;
                }
            }

            removed.push_back(remove);

            // check if all nodes have fewer than k neighbors
            bool found = false;
            for (u32 i = 0;i < graph.size() && !found;i++) {
                node* n = graph[i];
                found = n->c.size() >= k;
            }

            is_colorable = !found;
        }
        
        bool found = false;
        for (u32 i = 0;i < graph.size() && !found;i++) {
            node* n = graph[i];
            found = n->c.size() >= k;
        }

        is_colorable = !found;
        for (u32 i = 0;i < graph.size();i++) delete graph[i];
        return !is_colorable;
    }
};