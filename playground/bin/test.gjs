void main() {
	i32[] test;
	test.push(1);
	test.push(2);
	test.push(3);
	test.forEach((u32 idx, i32 e) : void => {
		print('t[' + idx.toFixed(1) + ']: ' + e.toFixed(1));
	});
	if (test.some((u32 idx, i32 e) : bool => { return e == 4; })) {
		print('yes');
	}
	
	i64 t = test.findIndex((u32 idx, i32 e) : bool => { return e == 3; });
	print(t.toFixed(1));

	// todo: handle null...
	i64 r = test.find((u32 idx, i32 e) : bool => { return idx == 2; });
	print(r.toFixed(1));
}