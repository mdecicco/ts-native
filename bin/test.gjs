format fmt = { a: f32, b: i32, c: u32 };
class vec3 {
    constructor (f32 x, f32 y, f32 z) {
        this.x = x;
        this.y = y;
        this.z = z;
        testvec(this);
        return this;
    }
    
    vec3 operator += (vec3 r) {
        this.x += r.x;
        this.y += r.y;
        this.z += r.z;
        return this;
    }
    
    f32 x;
    f32 y;
    f32 z;
};

i32 test(i32 x);
i32 something(i32 arg0, i32 arg1, i32 arg2) {
    i32 x = arg0 + arg1 * 4.0;
    i32 y = x - arg2;
    x = y++;
    for (i32 i = 0;i < 10;i++) {
        x += 3;
    }
    return test(x);
}
i32 test(i32 x) {
    return x > 5 ? x * 5 : x;
}
void pf(foo f) {
    print_foo(f);
    return;
}
i32 test1(i32 z) {
    foo a = new foo(0);
    vec3 b = new vec3(1.0, 2.0, 3.5);
    testvec(b);
    a.w = b.z;
    pf(a);
    delete a;
    testvec(b);
    delete b;
    return something(z, z + 1, z + 2);
}
i32 main(foo a) {
    a.x = 52;
    a.y *= 5;
    a.z = (6 / 3);
    a.w = 61.69;
    a.x = a;
    pf(a);
    return a.t(test1(100), a.x);
};
