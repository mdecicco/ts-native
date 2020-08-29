format fmt = { a: f32, b: i32, c: u32 };
class vec3 {
    constructor (f32 x, f32 y, f32 z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    destructor () {
        dtestvec(this);
    }
    
    vec3 operator + (vec3 r) {
        return vec3(this.x + r.x, this.y + r.y, this.z + r.z);
    }
    
    void operator = (vec3 r) {
        this.x = r.x;
        this.y = r.y;
        this.z = r.z;
    }
    
    f32 x;
    f32 y;
    f32 z;
};

void it() {
    testvec(vec3(0.0, 1.0, 2.0) + vec3(2.0, 1.0, 0.0));
}
