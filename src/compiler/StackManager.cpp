#include <tsn/compiler/StackManager.h>

#include <assert.h>

namespace tsn {
    namespace compiler {
        StackManager::StackManager() {
        }

        StackManager::~StackManager() {
        }

        void StackManager::reset() {
            m_slots.clear();
        }

        alloc_id StackManager::alloc(u32 sz) {
            // look for unused slot
            for (auto i = m_slots.begin();i != m_slots.end();i++) {
                if (i->in_use) continue;
                u64 s_sz = i->end - i->start;
                if (s_sz > sz) {
                    // split the slot
                    slot n = { i->start + sz, i->end, false };
                    i->end = i->start + sz;
                    i->in_use = true;
                    m_slots.insert(std::next(i), n);
                    return i->start;
                } else if (s_sz == sz) {
                    i->in_use = true;
                    return i->start;
                }
            }

            // create a new slot
            u32 out = (u32)m_slots.size() == 0 ? 0 : m_slots.back().end;
            m_slots.push_back({ out, out + sz, true });
            return out;
        }

        void StackManager::free(u32 addr) {
            for (auto i = m_slots.begin();i != m_slots.end();i++) {
                if (i->start == addr) {
                    i->in_use = false;
                    if (std::next(i) == m_slots.end()) {
                        if (i != m_slots.begin()) {
                            auto p = std::prev(i);
                            if (!p->in_use) {
                                m_slots.erase(i);
                                m_slots.erase(p);
                                return;
                            }
                        }
                        m_slots.erase(i);
                        return;
                    }

                    auto n = std::next(i);
                    if (!n->in_use) {
                        i->end = n->end;
                        m_slots.erase(n);
                    }

                    if (i != m_slots.begin()) {
                        auto p = std::prev(i);
                        if (!p->in_use) {
                            p->end = i->end;
                            m_slots.erase(i);
                        }
                    }

                    return;
                }
            }

            assert(false);
        }

        u32 StackManager::size() const {
            return m_slots.size() == 0 ? 0 : m_slots.back().end;
        }
    };
};