void main(bool ()(i32, i32) cb) {
	i32 x = 4;
	bool()(i32, i32) cb1 = (i32 a, i32 b) : bool => {
		return b == x;
	};
	if (cb1(3, 4)) {
		print('I AM GOING TO FUCKING KILL MYSELF');
	}
	if (cb(3, 3)) {
		print('I AM GOING TO FUCKING KILL MYSELF');
	}
}