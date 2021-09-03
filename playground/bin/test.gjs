import { vec2f } from 'math';

class some_guy {
	constructor(f32 x, f32 y) : v(x, y) { }
	constructor(some_guy g) : v(g.v) { }

	void print() {
		print('vec2f(' + this.v.x.toFixed(2) + ', ' + this.v.y.toFixed(2) + ')');
	}

	vec2f v;
};

i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
	return a + b;
};

i32 x = testCb(cb);
print(x.toFixed(2));

some_guy a = some_guy(2, 2);
a.print();

array<some_guy> arr;
for (u32 i = 0;i < 10;i++) {
	arr.push(some_guy(i, i));
}

for (u32 i = 0;i < arr.length;i++) arr[i].print();

void set_some_value(void()(i32) set) {
	set(69);
}