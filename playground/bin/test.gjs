void result(f32 x, i64 y) {
	print(y.toFixed(2) + ' iterations');
	print(x.toFixed(2));
}
void test() {
	f32 x = 1;
	i64 i = 0;
	for (;i < 99999999;i++) {
		f32 z = i;
		x = (z + z + 2.0f * z + 1.0f - 0.379f) / x;
	}

	result(x, i);
}