void test() {
	print('test');
}

class some {
	i32 x;

	void test() {
		print(this.x.toFixed(2));
	}

	static void ayy() {
		print('yo');
	}
};

some x;
x.x = 65;