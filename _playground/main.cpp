#include <tsn/gs.h>

#include <stdio.h>

using namespace tsn;
using namespace tsn::ffi;

struct test {
    test(i32 _a, i32 _b, i32 _c) : a(_a), b(_b), c(_c) {}
    ~test() { a = 1; b = 2; c = 3; }

    i32 set_all(i32 x) {
        return a = b = c = x;
    }

    i32 a, b, c;
};

i32 testFn(i32 arg, f32 x, const test& t, i32* hmm) {
    i32 r = printf("Yo %d, %f, (%d, %d, %d), %d\n", arg, x, t.a, t.b, t.c, *hmm);
    return r;
}

i32 squared(i32* self) {
    return *self * *self;
}
i32 square(i32* self) {
    return *self *= *self;
}

int main(i32 argc, const char** argv) {
    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    try {
        Context ctx;

        auto i_ = bind<i32>(&ctx, "i32");
        auto f  = bind<f32>(&ctx, "f32").finalize();
        auto v  = bind<void>(&ctx, "void").finalize();
        auto vp = bind<void*>(&ctx, "data").finalize();
        auto i  = i_.method("squared", squared).method("square", square).finalize();
        auto t  = bind<test>(&ctx, "test")
            .ctor<i32, i32, i32>()
            .dtor()
            .prop("a", &test::a)
            .prop("b", &test::b)
            .prop("c", &test::c)
            .prop<i32>("all", nullptr, &test::set_all)
        .finalize();

        Function* fn = bind(&ctx, "testFn", testFn);

        i32 x = 5;
        i32* y = &x;
        test o = test(4, 5, 6);

        Object testObj = Object(&ctx, t, 6, 1, 9);
        i32 o_c = testObj.prop("c").call("squared");
        o_c = testObj.prop("c");
        testObj.prop("c").call("square");
        o_c = testObj.prop("c");
        testObj.prop("all", 61);
        testObj.prop("b", 69);

        i32 r = 0;
        r = call(&ctx, fn, 1, 1.25f, test(1, 2, 3), x);
        r = call(&ctx, fn, 2, 1.25f, o, y);
        r = call(&ctx, fn, 3, 1.25f, &o, 5);
        r = call(&ctx, fn, 3, 1.25f, testObj, 5);
        r = call(&ctx, fn, 3, 1.25f, Object(&ctx, t, 4, 2, 0), 5);
        r = call(&ctx, fn, 3, 1.25f, &testObj, 5);
    } catch (const std::exception& e) {
        printf("Exception: %s\n", e.what());
    }

    utils::String::Allocator::Destroy();
    utils::Mem::Destroy();
    return 0;
}