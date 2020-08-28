format fmt = { a: f32, b: i32, c: u32 };
class vec3 {
    constructor (f32 x, f32 y, f32 z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    
    vec3 operator + (vec3 r) {
        return new vec3(this.x + r.x, this.y + r.y, this.z + r.z);
    }
    
    f32 x;
    f32 y;
    f32 z;
};

void it() {
    vec3 a = new vec3(1.0, 2.0, 3.0);
    vec3 b = new vec3(3.0, 2.0, 1.0);
    testvec(a + b);
}
