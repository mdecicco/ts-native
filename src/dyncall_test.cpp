#include <stdio.h>
#include <util/util.h>
#include <bind/bind.h>
#include <common/script_type.h>
#include <dyncall_signature.h>

void tasts(float f, int i) {
    printf("%d, %f\n", i, f);
}

class a {
    a(float _x) : x(_x) { }
    ~a() { }

    float mul(float y) { return x * y; }

    float x;
};

int t1(float f) {
    return printf("ass %f\n", f);
}

struct ts {
    int x,y,z,w;
};

ts t2(int y) {
    return { 1, 2, 3, 4 };
}

void t3(ts& x) {
    printf("ass %d, %d, %d, %d\n", x.x, x.y, x.z, x.w);
}

double t4(ts& values) {
	return values.x + values.y + values.z + values.w;
}

void dyncall_test(gjs::type_manager* tpm) {
    gjs::bind::wrap_class<ts>(tpm, "ts").prop("x", &ts::x).prop("y", &ts::y).prop("z", &ts::z).prop("w", &ts::w).finalize();
    
    gjs::bind::wrapped_function* t1_f = gjs::bind::wrap(tpm, "t1", t1);
    gjs::bind::wrapped_function* t2_f = gjs::bind::wrap(tpm, "t2", t2);
    gjs::bind::wrapped_function* t3_f = gjs::bind::wrap(tpm, "t3", t3);
    gjs::bind::wrapped_function* t4_f = gjs::bind::wrap(tpm, "t4", t4);
    
    float f = 3.14f;
    int result;
    void* args1[] = { *(void**)&f };
    t1_f->call(&result, args1);

    ts t2;
    void* args2[] = { (void*)5 };
    t2_f->call(&t2, args2);

    void* args3[] = { &t2 };
    t3_f->call(nullptr, args3);

    double sum;
    ts t4 = { 5, 4, 3, 2 };
    void* args4[] = { &t4 };
    t4_f->call(&sum, args4);

    return;
}