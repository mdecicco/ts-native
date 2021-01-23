import { vec2f, vec3f, vec4f, random } from 'math';

format f0 {
	i32 a,
	i32 b,
	i32 c
};

format f1 {
	f0 a,
	f0 b,
	f0 c
};

f1 y = f1({
	a: f0({ a: 1, b: 2, c: 3 }),
	b: f0({ a: 4, b: 5, c: 6 }),
	c: f0({ a: 7, b: 8, c: 9 })
});

class t {
	constructor(i32 f) {
		this.x = f;
	}

	void print() {
		print(this.x.toFixed(2));
	}

	i32 x;
};

f1 func() {
	return y;
}

vec2f vec() {
	return vec2f(2, 5);
}

vec4f v4() {
	return random(vec4f(0.0f, 0.0f, 0.0f, 0.0f), vec4f(1.0f, 1.0f, 1.0f, 1.0f));
}