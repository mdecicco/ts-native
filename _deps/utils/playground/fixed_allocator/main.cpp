#include <utils/Allocator.h>
#include <utils/Timer.h>

#include <vector>

using namespace utils;
class TestAllocator : public FixedAllocator<u32> {
    public:
        TestAllocator() : FixedAllocator<u32>(128) { }
        ~TestAllocator() { }

    protected:
        virtual void* allocInternal(u32 count) {
            void* out = FixedAllocator<u32>::allocInternal(count);

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

            return out;
        }

        virtual size_t freeInternal(void* ptr) {
            size_t out = FixedAllocator<u32>::freeInternal(ptr);

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

            return out;
        }
};

int main(int argc, const char** argv) {
    TestAllocator a;
    std::vector<u32*> ptrs;

    Timer t;
    t.start();
    while (true) {
        if (t > 0.05f) {
            t.reset();
            t.start();
        } else continue;

        system("cls");

        bool did_something = false;
        while (!did_something) {
            if (rand() % 2 == 0) {
                // alloc
                u32* p = a.alloc(u32(rand()) % 16);
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
    }

    return 0;
}
