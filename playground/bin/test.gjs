import { test } from 'importme';

export void main() {
	test(5, 6, 7);
	for (u32 i = 0;i < process.argc;i++) {
		string name = process.get_arg(i).name;
		string value = process.get_arg(i).value;
		print(name + ' : ' + value);
	}
}