import 'test1' as t;

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

enum test_enum {
	a = 10 / 2,
	b,
	c,
	d,
	e,
	f
};

t.test();
t.some.ayy();

t.some x;
x.x = 5;
x.test();