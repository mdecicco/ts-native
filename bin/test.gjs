void it() {
    array<f32> x;
    x.push(1.23f);
    x.push(x[0]);
    foo f;
    f.x = 5;
    f.x += 5;
    if (x.length > 0) {
        x.push(5.55f);
    }
}
