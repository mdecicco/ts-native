bool test (bool ()(i32, i32) cb) {
	return cb(4, 3);
}

u64 main(u64 a) {
	fuck((i32 b, i32 c) : bool => {
		print('b: ' + b.toFixed(2) + ', c: ' + c.toFixed(2));
		return b == c;
	});
	bool dilg = test((i32 b, i32 c) : bool => {
		return b == c;
	});
	if (dilg) print('testicles');
	return a + 1;
}