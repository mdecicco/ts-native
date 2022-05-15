#include <utils/Allocator.h>
#include <utils/Timer.h>

#include <vector>

using namespace utils;
class TestAllocator : public FixedAllocator<u32> {
    public:
        TestAllocator(u32 a, u32 b = 0) : FixedAllocator<u32>(a, b) { }
        virtual ~TestAllocator() { }

        void debug() {
            auto n = m_memList;
            u32 nid = 0;
            while (n) {
                for (u32 i = 0;i < n->count;i++) {
                    if (n->used) printf("%c", 'a' + nid);
                    else printf("-");
                }
                n = n->next;
                nid++;
            }
            printf("\n");
        }
};

class TestPagedAllocator : public PagedAllocator<u32, TestAllocator> {
    public:
        TestPagedAllocator(u32 a, u32 b = 0) : PagedAllocator<u32, TestAllocator>(a, b) {
        }

        virtual ~TestPagedAllocator() {
        }

        void debug() {
            page_node* n = m_pages;
            while (n) {
                n->allocator->debug();
                n = n->next;
            }
        }
};

int main(int argc, const char** argv) {
    TestPagedAllocator a(128, 128);
    std::vector<u32*> ptrs;

    Timer t;

    t.start();
    while (true) {
        if (t > 1.05f) {
            t.reset();
            t.start();
        } else continue;

        bool did_something = false;
        while (!did_something) {
            if (rand() % 2 == 0) {
                // alloc
                u32* p = a.alloc((u32(rand()) % 63) + 1);
                if (p) {
                    ptrs.push_back(p);
                    did_something = true;
                }
            } else {
                // dealloc
                if (ptrs.size() > 0) {
                    u32 idx = u32(rand()) % ptrs.size();
                    a.free(ptrs[idx]);
                    ptrs.erase(ptrs.begin() + idx);
                    did_something = true;
                }
            }
        }

        system("cls");
        a.debug();
    }

    return 0;
}
