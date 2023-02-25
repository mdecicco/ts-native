#include "../catch.hpp"
#include <tsn/gs.h>
using namespace tsn;
using namespace tsn::ffi;

i32 a, b, c;

void reset() { a = b = c = 0; }
void _expectVal(i32 _a, i32 _b, i32 _c) {
    a = _a;
    b = _b;
    c = _c;
}
void _expectPtr(i32* _a, i32* _b, i32* _c) {
    a = *_a;
    b = *_b;
    c = *_c;
}
void _expectRef(i32& _a, i32& _b, i32& _c) {
    a = _a;
    b = _b;
    c = _c;
}
void i32_setVal(i32* self, i32 value) {
    *self = value;
}
void i32_setPtr(i32* self, i32* value) {
    *self = *value;
}
void i32_setRef(i32* self, i32& value) {
    *self = value;
}

struct _h2h_test {
    i32 d, e, f;
    void reset() { d = e = f = 0; }
    void expectVal(i32 _d, i32 _e, i32 _f) {
        d = _d;
        e = _e;
        f = _f;
    }
    void expectPtr(i32* _d, i32* _e, i32* _f) {
        d = *_d;
        e = *_e;
        f = *_f;
    }
    void expectRef(i32& _d, i32& _e, i32& _f) {
        d = _d;
        e = _e;
        f = _f;
    }
};

struct _h2h_ctor_val_test {
    i32 g, h, i;
    _h2h_ctor_val_test(i32 _g, i32 _h, i32 _i) {
        g = _g;
        h = _h;
        i = _i;
    }
};

struct _h2h_ctor_ptr_test {
    i32 g, h, i;
    _h2h_ctor_ptr_test(i32* _g, i32* _h, i32* _i) {
        g = *_g;
        h = *_h;
        i = *_i;
    }
};

struct _h2h_ctor_ref_test {
    i32 g, h, i;
    _h2h_ctor_ref_test(i32& _g, i32& _h, i32& _i) {
        g = _g;
        h = _h;
        i = _i;
    }
};

TEST_CASE("FFI", "[host to host arguments]") {
    Context ctx;

    bind<void>(&ctx, "void").finalize();
    bind<void*>(&ctx, "ptr").finalize();
    DataType* i32_tp = bind<i32>(&ctx, "i32")
        .method("setVal", i32_setVal)
        .method("setPtr", i32_setPtr)
        .method("setRef", i32_setRef)
        .staticMethod("expectVal", _expectVal)
        .staticMethod("expectPtr", _expectPtr)
        .staticMethod("expectRef", _expectRef)
    .finalize();
    DataType* test_tp = bind<_h2h_test>(&ctx, "test")
        .method("expectVal", &_h2h_test::expectVal)
        .method("expectPtr", &_h2h_test::expectPtr)
        .method("expectRef", &_h2h_test::expectRef)
        .staticMethod("staticExpectVal", _expectVal)
        .staticMethod("staticExpectPtr", _expectPtr)
        .staticMethod("staticExpectRef", _expectRef)
    .finalize();

    DataType* test_val_ctor_tp = bind<_h2h_ctor_val_test>(&ctx, "val_ctor")
        .ctor<i32, i32, i32>()
        .prop("a", &_h2h_ctor_val_test::g)
        .prop("b", &_h2h_ctor_val_test::h)
        .prop("c", &_h2h_ctor_val_test::i)
    .finalize();

    DataType* test_ptr_ctor_tp = bind<_h2h_ctor_ptr_test>(&ctx, "ptr_ctor")
        .ctor<i32*, i32*, i32*>()
        .prop("a", &_h2h_ctor_ptr_test::g)
        .prop("b", &_h2h_ctor_ptr_test::h)
        .prop("c", &_h2h_ctor_ptr_test::i)
    .finalize();

    DataType* test_ref_ctor_tp = bind<_h2h_ctor_ref_test>(&ctx, "ref_ctor")
        .ctor<i32&, i32&, i32&>()
        .prop("a", &_h2h_ctor_ref_test::g)
        .prop("b", &_h2h_ctor_ref_test::h)
        .prop("c", &_h2h_ctor_ref_test::i)
    .finalize();

    Function* expectVal = bind(&ctx, "expectVal", _expectVal);
    Function* expectPtr = bind(&ctx, "expectPtr", _expectPtr);
    Function* expectRef = bind(&ctx, "expectRef", _expectRef);
    
    SECTION("Callee is a global function") {
        SECTION("Callee has signature void (*)(i32, i32, i32)") {
            Function* f = expectVal;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                call(&ctx, f, 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                call(&ctx, f, &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                call(&ctx, f, a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                call(&ctx, f, pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                call(&ctx, f, oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                call(&ctx, f, &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                call(&ctx, f, poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32*, i32*, i32*)") {
            Function* f = expectPtr;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                call(&ctx, f, 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                call(&ctx, f, &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                call(&ctx, f, a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                call(&ctx, f, pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                call(&ctx, f, oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                call(&ctx, f, &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                call(&ctx, f, poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32&, i32&, i32&)") {
        Function* f = expectRef;
        SECTION("Caller passes (i32, i32, i32)") {
            reset();
            call(&ctx, f, 1, 2, 3);

            REQUIRE(a == 1);
            REQUIRE(b == 2);
            REQUIRE(c == 3);
        }

        SECTION("Caller passes (i32*, i32*, i32*)") {
            i32 a1 = 4;
            i32 b1 = 5;
            i32 c1 = 6;

            reset();
            call(&ctx, f, &a1, &b1, &c1);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }
        
        SECTION("Caller passes (i32&, i32&, i32&)") {
            i32 a1 = 7;
            i32 b1 = 8;
            i32 c1 = 9;

            reset();
            call(&ctx, f, a1, b1, c1);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }
        
        SECTION("Caller passes (i32*&, i32*&, i32*&)") {
            i32 a1 = 10;
            i32 b1 = 11;
            i32 c1 = 12;

            i32 *pa = &a1;
            i32 *pb = &b1;
            i32 *pc = &c1;

            reset();
            call(&ctx, f, pa, pb, pc);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }
    
        SECTION("Caller passes (Object&, Object&, Object&)") {
            i32 a1 = 13;
            i32 b1 = 14;
            i32 c1 = 15;

            Object oa = Object(&ctx, false, i32_tp, &a1);
            Object ob = Object(&ctx, false, i32_tp, &b1);
            Object oc = Object(&ctx, false, i32_tp, &c1);

            reset();
            call(&ctx, f, oa, ob, oc);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }

        SECTION("Caller passes (Object*, Object*, Object*)") {
            i32 a1 = 16;
            i32 b1 = 17;
            i32 c1 = 18;
            
            Object oa = Object(&ctx, false, i32_tp, &a1);
            Object ob = Object(&ctx, false, i32_tp, &b1);
            Object oc = Object(&ctx, false, i32_tp, &c1);

            reset();
            call(&ctx, f, &oa, &ob, &oc);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }

        SECTION("Caller passes (Object*&, Object*&, Object*&)") {
            i32 a1 = 19;
            i32 b1 = 20;
            i32 c1 = 21;
            
            Object oa = Object(&ctx, false, i32_tp, &a1);
            Object ob = Object(&ctx, false, i32_tp, &b1);
            Object oc = Object(&ctx, false, i32_tp, &c1);

            Object* poa = &oa;
            Object* pob = &ob;
            Object* poc = &oc;

            reset();
            call(&ctx, f, poa, pob, poc);

            REQUIRE(a == a1);
            REQUIRE(b == b1);
            REQUIRE(c == c1);
        }
    }
    }

    SECTION("Callee is a class method") {
        _h2h_test _test;
        Object test = Object(&ctx, false, test_tp, &_test);

        SECTION("Callee has signature void (class::*)(i32, i32, i32)") {
            SECTION("Caller passes (i32, i32, i32)") {
                _test.reset();
                test.call("expectVal", 1, 2, 3);

                REQUIRE(_test.d == 1);
                REQUIRE(_test.e == 2);
                REQUIRE(_test.f == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                _test.reset();
                test.call("expectVal", &a1, &b1, &c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                _test.reset();
                test.call("expectVal", a1, b1, c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                _test.reset();
                test.call("expectVal", pa, pb, pc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectVal", oa, ob, oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectVal", &oa, &ob, &oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                _test.reset();
                test.call("expectVal", poa, pob, poc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
        }
        
        SECTION("Callee has signature void (class::*)(i32*, i32*, i32*)") {
            SECTION("Caller passes (i32, i32, i32)") {
                _test.reset();
                test.call("expectPtr", 1, 2, 3);

                REQUIRE(_test.d == 1);
                REQUIRE(_test.e == 2);
                REQUIRE(_test.f == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                _test.reset();
                test.call("expectPtr", &a1, &b1, &c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                _test.reset();
                test.call("expectPtr", a1, b1, c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                _test.reset();
                test.call("expectPtr", pa, pb, pc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectPtr", oa, ob, oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectPtr", &oa, &ob, &oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                _test.reset();
                test.call("expectPtr", poa, pob, poc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
        }
        
        SECTION("Callee has signature void (class::*)(i32&, i32&, i32&)") {
            SECTION("Caller passes (i32, i32, i32)") {
                _test.reset();
                test.call("expectRef", 1, 2, 3);

                REQUIRE(_test.d == 1);
                REQUIRE(_test.e == 2);
                REQUIRE(_test.f == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                _test.reset();
                test.call("expectRef", &a1, &b1, &c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                _test.reset();
                test.call("expectRef", a1, b1, c1);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                _test.reset();
                test.call("expectRef", pa, pb, pc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectRef", oa, ob, oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                _test.reset();
                test.call("expectRef", &oa, &ob, &oc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                _test.reset();
                test.call("expectRef", poa, pob, poc);

                REQUIRE(_test.d == a1);
                REQUIRE(_test.e == b1);
                REQUIRE(_test.f == c1);
            }
        }
    }
    
    SECTION("Callee is a static class method") {
        _h2h_test _test;
        Object test = Object(&ctx, false, test_tp, &_test);

        SECTION("Callee has signature void (*)(i32, i32, i32)") {
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("staticExpectVal", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("staticExpectVal", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("staticExpectVal", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("staticExpectVal", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectVal", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectVal", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("staticExpectVal", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32*, i32*, i32*)") {
            Function* f = expectPtr;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("staticExpectPtr", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("staticExpectPtr", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("staticExpectPtr", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("staticExpectPtr", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectPtr", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectPtr", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("staticExpectPtr", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32&, i32&, i32&)") {
            Function* f = expectRef;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("staticExpectRef", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("staticExpectRef", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("staticExpectRef", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("staticExpectRef", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectRef", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("staticExpectRef", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("staticExpectRef", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
    }

    SECTION("Callee is a primitive pseudo-method") {
        i32 _test;
        Object test = Object(&ctx, false, i32_tp, &_test);

        SECTION("Callee has signature void (*)(i32)") {
            SECTION("Caller passes (i32)") {
                _test = 0;
                test.call("setVal", 1);

                REQUIRE(_test == 1);
            }

            SECTION("Caller passes (i32*)") {
                i32 a1 = 2;

                _test = 0;
                test.call("setVal", &a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32&)") {
                i32 a1 = 3;

                _test = 0;
                test.call("setVal", a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32*&)") {
                i32 a1 = 4;
                i32 *pa = &a1;

                _test = 0;
                test.call("setVal", pa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object&)") {
                i32 a1 = 5;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setVal", oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*)") {
                i32 a1 = 6;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setVal", &oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*&)") {
                i32 a1 = 7;
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object* poa = &oa;

                _test = 0;
                test.call("setVal", poa);

                REQUIRE(_test == a1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32*)") {
            SECTION("Caller passes (i32)") {
                _test = 0;
                test.call("setPtr", 1);

                REQUIRE(_test == 1);
            }

            SECTION("Caller passes (i32*)") {
                i32 a1 = 2;

                _test = 0;
                test.call("setPtr", &a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32&)") {
                i32 a1 = 3;

                _test = 0;
                test.call("setPtr", a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32*&)") {
                i32 a1 = 4;
                i32 *pa = &a1;

                _test = 0;
                test.call("setPtr", pa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object&)") {
                i32 a1 = 5;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setPtr", oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*)") {
                i32 a1 = 6;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setPtr", &oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*&)") {
                i32 a1 = 7;
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object* poa = &oa;

                _test = 0;
                test.call("setPtr", poa);

                REQUIRE(_test == a1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32&)") {
            SECTION("Caller passes (i32)") {
                _test = 0;
                test.call("setRef", 1);

                REQUIRE(_test == 1);
            }

            SECTION("Caller passes (i32*)") {
                i32 a1 = 2;

                _test = 0;
                test.call("setRef", &a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32&)") {
                i32 a1 = 3;

                _test = 0;
                test.call("setRef", a1);

                REQUIRE(_test == a1);
            }
            
            SECTION("Caller passes (i32*&)") {
                i32 a1 = 4;
                i32 *pa = &a1;

                _test = 0;
                test.call("setRef", pa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object&)") {
                i32 a1 = 5;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setRef", oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*)") {
                i32 a1 = 6;
                Object oa = Object(&ctx, false, i32_tp, &a1);

                _test = 0;
                test.call("setRef", &oa);

                REQUIRE(_test == a1);
            }

            SECTION("Caller passes (Object*&)") {
                i32 a1 = 7;
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object* poa = &oa;

                _test = 0;
                test.call("setRef", poa);

                REQUIRE(_test == a1);
            }
        }
    }

    SECTION("Callee is a static primitive pseudo-method") {
        i32 _test;
        Object test = Object(&ctx, false, i32_tp, &_test);

        SECTION("Callee has signature void (*)(i32, i32, i32)") {
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("expectVal", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("expectVal", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("expectVal", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("expectVal", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectVal", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectVal", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("expectVal", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32*, i32*, i32*)") {
            Function* f = expectPtr;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("expectPtr", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("expectPtr", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("expectPtr", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("expectPtr", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectPtr", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectPtr", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("expectPtr", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32&, i32&, i32&)") {
            Function* f = expectRef;
            SECTION("Caller passes (i32, i32, i32)") {
                reset();
                test.call("expectRef", 1, 2, 3);

                REQUIRE(a == 1);
                REQUIRE(b == 2);
                REQUIRE(c == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                reset();
                test.call("expectRef", &a1, &b1, &c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                reset();
                test.call("expectRef", a1, b1, c1);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                reset();
                test.call("expectRef", pa, pb, pc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        
            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectRef", oa, ob, oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                reset();
                test.call("expectRef", &oa, &ob, &oc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                reset();
                test.call("expectRef", poa, pob, poc);

                REQUIRE(a == a1);
                REQUIRE(b == b1);
                REQUIRE(c == c1);
            }
        }
    }

    SECTION("Callee is a class constructor") {
        SECTION("Callee has signature void (*)(i32, i32, i32)") {
            DataType* tp = test_val_ctor_tp;
            SECTION("Caller passes (i32, i32, i32)") {
                Object o = Object(&ctx, tp, 1, 2, 3);
                REQUIRE(((i32)o.prop("a")) == 1);
                REQUIRE(((i32)o.prop("b")) == 2);
                REQUIRE(((i32)o.prop("c")) == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                Object o = Object(&ctx, tp, &a1, &b1, &c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                Object o = Object(&ctx, tp, a1, b1, c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                Object o = Object(&ctx, tp, pa, pb, pc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, oa, ob, oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, &oa, &ob, &oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                Object o = Object(&ctx, tp, poa, pob, poc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32*, i32*, i32*)") {
            DataType* tp = test_ptr_ctor_tp;
            SECTION("Caller passes (i32, i32, i32)") {
                Object o = Object(&ctx, tp, 1, 2, 3);
                REQUIRE(((i32)o.prop("a")) == 1);
                REQUIRE(((i32)o.prop("b")) == 2);
                REQUIRE(((i32)o.prop("c")) == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                Object o = Object(&ctx, tp, &a1, &b1, &c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                Object o = Object(&ctx, tp, a1, b1, c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                Object o = Object(&ctx, tp, pa, pb, pc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, oa, ob, oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, &oa, &ob, &oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                Object o = Object(&ctx, tp, poa, pob, poc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
        }
        
        SECTION("Callee has signature void (*)(i32&, i32&, i32&)") {
            DataType* tp = test_ref_ctor_tp;
            SECTION("Caller passes (i32, i32, i32)") {
                Object o = Object(&ctx, tp, 1, 2, 3);
                REQUIRE(((i32)o.prop("a")) == 1);
                REQUIRE(((i32)o.prop("b")) == 2);
                REQUIRE(((i32)o.prop("c")) == 3);
            }

            SECTION("Caller passes (i32*, i32*, i32*)") {
                i32 a1 = 4;
                i32 b1 = 5;
                i32 c1 = 6;

                Object o = Object(&ctx, tp, &a1, &b1, &c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32&, i32&, i32&)") {
                i32 a1 = 7;
                i32 b1 = 8;
                i32 c1 = 9;

                Object o = Object(&ctx, tp, a1, b1, c1);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
            
            SECTION("Caller passes (i32*&, i32*&, i32*&)") {
                i32 a1 = 10;
                i32 b1 = 11;
                i32 c1 = 12;

                i32 *pa = &a1;
                i32 *pb = &b1;
                i32 *pc = &c1;

                Object o = Object(&ctx, tp, pa, pb, pc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object&, Object&, Object&)") {
                i32 a1 = 13;
                i32 b1 = 14;
                i32 c1 = 15;

                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, oa, ob, oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*, Object*, Object*)") {
                i32 a1 = 16;
                i32 b1 = 17;
                i32 c1 = 18;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object o = Object(&ctx, tp, &oa, &ob, &oc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }

            SECTION("Caller passes (Object*&, Object*&, Object*&)") {
                i32 a1 = 19;
                i32 b1 = 20;
                i32 c1 = 21;
                
                Object oa = Object(&ctx, false, i32_tp, &a1);
                Object ob = Object(&ctx, false, i32_tp, &b1);
                Object oc = Object(&ctx, false, i32_tp, &c1);

                Object* poa = &oa;
                Object* pob = &ob;
                Object* poc = &oc;

                Object o = Object(&ctx, tp, poa, pob, poc);
                REQUIRE(((i32)o.prop("a")) == a1);
                REQUIRE(((i32)o.prop("b")) == b1);
                REQUIRE(((i32)o.prop("c")) == c1);
            }
        }
    }
}