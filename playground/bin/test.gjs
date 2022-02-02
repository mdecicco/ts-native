extern 'test.dll' {
	void test(i32 a, i32 b, i32 c);
}

void main() {
	test(5, 6, 7);
}