i32()(i32) addFunc(i32 a) {
	return (i32 b) : i32 => {
		print('a was ' + a.toFixed(2));
		print('b was ' + b.toFixed(2));
		return a + b;
	};
}

void set_some_value(void()(i32) set) {
	pointer<i32> val;
	val.reset(0);
	i32 x = 68;
	void ()() outer = () : void => {
		void ()() inner = () : void => {
			print('x was ' + x.toFixed(2));
			i32()(i32) add = addFunc(1);
			i32 z = add(x);
			print('z was ' + z.toFixed(2));
			val = z;
		};
		inner();
	};

	outer();
	
	set(val.value);
}