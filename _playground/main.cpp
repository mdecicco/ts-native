#include <gs/gs.h>

#include <stdio.h>

using namespace gs;
using namespace gs::ffi;

struct test {
    test(i32 _a, i32 _b, i32 _c) : a(_a), b(_b), c(_c) {}
    ~test() { a = 1; b = 2; c = 3; }
    i32 a, b, c;
};

i32 testFn(i32 arg, f32 x, const test& t, i32* hmm) {
    i32 r = printf("Yo %d, %f, (%d, %d, %d), %d\n", arg, x, t.a, t.b, t.c, *hmm);
    return r;
}

int main(i32 argc, const char** argv) {
    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    try {
        Context ctx;

        auto i  = bind<i32>(&ctx, "i32").finalize();
        auto f  = bind<f32>(&ctx, "f32").finalize();
        auto v  = bind<void>(&ctx, "void").finalize();
        auto vp = bind<void*>(&ctx, "data").finalize();
        auto t  = bind<test>(&ctx, "test").ctor<i32, i32, i32>().dtor().finalize();

        Function* fn = bind(&ctx, "testFn", testFn);

        i32 x = 5;
        i32* y = &x;
        test o = test(4, 5, 6);
        Object testObj = Object(ctx.getTypes(), t, 6, 1, 9);
        i32 r = 0;
        r = call(fn, 1, 1.25f, test(1, 2, 3), x);
        r = call(fn, 2, 1.25f, o, y);
        r = call(fn, 3, 1.25f, &o, 5);
        r = call(fn, 3, 1.25f, testObj, 5);
        r = call(fn, 3, 1.25f, &testObj, 5);
        r = call(fn, 3, 1.25f, Object(ctx.getTypes(), t, 4, 2, 0), 5);

        printf("Test\n");
    } catch (const std::exception& e) {
        printf("Exception: %s\n", e.what());
    }

    utils::String::Allocator::Destroy();
    utils::Mem::Destroy();
    return 0;
}