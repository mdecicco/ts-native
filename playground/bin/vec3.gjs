class vec3 {
    constructor(f32 _x) {
        this.x = _x;
        this.y = _x;
        this.z = _x;
    }
    constructor(vec3 t) {
        this.x = t.x;
        this.y = t.y;
        this.z = t.z;
    }

    f32 x;
    f32 y;
    f32 z;
};
