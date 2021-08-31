import { vec2f, vec3f, vec4f, random } from 'math';

i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
	return a + b;
};

i32 x = testCb(cb);
print(x.toFixed(2));