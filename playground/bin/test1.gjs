void main() {
	f64 x = 1.0;
	f64 z = 0.0;
	for (i32 i = 0;i < 99999999;i++) {
		z = i;
		x = (z + z + 2.0 * z + 1.0 - 0.379) / x;
	}

	print(x.toFixed(5));
}