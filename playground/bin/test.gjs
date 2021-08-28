import { vec2f, vec3f, vec4f, random } from 'math';

void tttt(i32 a, i8 b, f32 c) {
	print(a.toFixed(2));
	print(b.toFixed(2));
	print(c.toFixed(2));
}

test_cb(tttt);

pointer<f32> a;
a.reset(1.234f);
pointer<f32> b;
b.share(a);
b.value = 3.14f;

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

class ndc {
	constructor(i32 a, i32 b) {
		this.a = a;
		this.b = b;
	}

	i32 a;
	i32 b;
};

class t {
	constructor(i32 f) : x(f), test(ndc(0, 0)) {
	}

	void print() {
		print(this.x.toFixed(2));
	}

	i32 x;
	ndc test;
};

array<vec2f> arr;
arr.push(vec2f(1.0f, 2.0f));
arr.push(vec2f(3.0f, 4.0f));

f1 func() {
	return y;
}

vec2f vec() {
	return arr[1];
}

vec2f vec1() {
	return vec2f(a.value, b.value);
}

vec4f v4() {
	return random(vec4f(0.0f, 0.0f, 0.0f, 0.0f), vec4f(1.0f, 1.0f, 1.0f, 1.0f));
}