class test<subtype> {
    constructor(subtype _x) {
        this.x = _x;
        this.y = _x;
        this.z = _x;
    }
    constructor(test<subtype> t) {
        this.x = t.x;
        this.y = t.y;
        this.z = t.z;
    }

    void print() {
        print_f32(166, this.x);
        print_f32(616, this.x);
        print_f32(661, this.x);
    }

    subtype x;
    subtype y;
    subtype z;
};

void it() {
    array<f32> x;
    x.push(1.23f);
    x.push(x[0]);
    x[0] = 4.56f;

    if (x.length > 0) {
        x.push(5.55f);
    }

    for(u8 i = 0;i < 10;i++) {
        x.push(i);
    }

    test<f32> ayy = test<f32>(1.23f);
    ayy.print();

    for(u8 i = 0;i < x.length;i++) {
        print_f32(i, x[i]);
    }
}
