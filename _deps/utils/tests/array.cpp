#include <utils/Array.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace utils;

void printArr(const char* name, const Array<u32>& arr) {
    if (name) printf("%-60s: [", name);
    else printf("[");
    arr.each([](u32 ele, u32 idx) {
        if (idx > 0) printf(", ");
        printf("%u", ele);
        return;
    });
    if (name) printf("]\n\n");
    else printf("]");
}

void printView(const char* name, const ArrayView<u32>& arr) {
    printf("%-60s: [", name);
    arr.each([](u32 ele, u32 idx) {
        if (idx > 0) printf(", ");
        printf("%u", ele);
        return;
    });
    printf("] (view of ");
    printArr(nullptr, *arr.target());
    printf(")\n\n");
}

void printNum(const char* name, i64 n) {
    printf("%-60s: %lld\n\n", name, n);
}

void printStr(const char* name, const char* s) {
    printf("%-60s: %s\n\n", name, s);
}

TEST_CASE("Arrays", "[array]") {
    SECTION("TODO rewrite all this") {
    }
}
