#include <utils/Array.hpp>
#include <utils/String.h>

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

int main(int argc, const char** argv) {
    Array<u32> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    printArr("Initial", arr);

    arr.remove(3, 5);
    printArr("After '.remove(3, 5)'", arr);

    arr.insert(3, { 3, 4, 5, 6, 7 });
    printArr("After '.insert(3, { 3, 4, 5, 6, 7 })'", arr);

    Array<u32> extracted = arr.extract(3, 5);
    printArr("After '.extract(3, 5)'", arr);
    printArr("Extracted", extracted);

    u32 v = arr.pop();
    printArr("After '.pop()'", arr);
    printNum("Result", v);

    v = arr.pop(true);
    printArr("After '.pop(true)'", arr);
    printNum("Result", v);

    printArr("arr.concat(extracted)", arr.concat(extracted));
    printArr("After 'arr.concat(extracted)'", arr);

    arr.append({ 6, 1, 6, 9 });
    printArr("After 'arr.append({ 6, 1, 6, 9 })'", arr);

    printStr("arr.some([](u32 e) { return e == 8; })", arr.some([](u32 e) { return e == 8; }) ? "true" : "false");
    printStr("arr.some([](u32 e) { return e == 10; })", arr.some([](u32 e) { return e == 10; }) ? "true" : "false");
    printNum("arr.findIndex([](u32 e) { return e == 9; })", arr.findIndex([](u32 e) { return e == 9; }));

    u32* ele = arr.find([](u32 e) { return (e / 3) == 3; });
    printStr("arr.find([](u32 e) { return (e / 3) == 3; })", ele ? String::Format("%d", *ele).c_str() : "null");
    ele = arr.find([](u32 e) { return e == 10; });
    printStr("arr.find([](u32 e) { return e == 10; })", ele ? String::Format("%d", *ele).c_str() : "null");

    printArr("arr.filter([](u32 e) { return e % 2 == 0; })", arr.filter([](u32 e) { return e % 2 == 0; }));
    printArr("arr.map([](u32 e) { return e * 2; })", arr.map([](u32 e) { return e * 2; }));

    ArrayView<u32> vw = arr.view(3, 10);
    printView("ArrayView<u32> vw = arr.view(3, 10)", vw);

    arr.append({ 1, 2, 3 });
    printView("vw after 'arr.append({ 1, 2, 3 })'", vw);

    arr.remove(3, 4);
    printView("vw after 'arr.remove(3, 4)'", vw);

    vw.sort([](u32 a, u32 b) { return a < b; });
    printArr("After 'vw.sort([](u32 a, u32 b) { return a < b; })'", arr);

    vw.each([](u32& e) { e = 2; });
    printArr("After 'vw.each([](u32& e) { e = 2; })'", arr);

    arr.sort([](u32 a, u32 b) { return a < b; });
    printArr("After 'arr.sort([](u32 a, u32 b) { return a < b; })'", arr);

    return 0;
}
