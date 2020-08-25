#include <allocator.h>
#include <context.h>

using namespace gjs;

class foo {
	public:
		foo(i32* _x) { x = _x; w = 3.0f; }
		~foo() { }

		i32 t(i32 a) {
			return printf("%d, %d\n", y, a);
		}

		i32* x;
		i32 y;
		i32 z;
		f32 w;
};

void print_foo(const foo& f) {
	printf("foo: %d, %d, %d, %f\n", *f.x, f.y, f.z, f.w);
}

int main(int arg_count, const char** args) {
	const char* src =
		"i32 test(i32 x);\n"
		"i32 something(i32 arg0, i32 arg1, i32 arg2) {\n"
		"    i32 x = arg0 + arg1 * 4.0;\n"
		"    i32 y = x - arg2;\n"
		"    x = y++;\n"
		"    for (i32 i = 0;i < 10;i++) {\n"
		"        x += 3;\n"
		"    }\n"
		"    return test(x);\n"
		"}\n"
		"i32 test(i32 x) {\n"
		"    return x * 5;\n"
		"}\n"
		"i32 test1(i32 z) {\n"
		"    return something(z, z + 1, z + 2);\n"
		"}\n"
		"i32 main(foo a) {\n"
		"    a.x = 52;\n"
		"    a.y *= 5;\n"
		"    a.z = 2;\n"
		"    a.w = 61.69;\n"
		"    print_foo(a);\n"
		"    return a.t(test1(100));\n"
		"}\n";

	vm_allocator* alloc = new basic_malloc_allocator();
	vm_context ctx(alloc, 4096, 4096);
	ctx.log_exceptions(true);
	ctx.log_instructions(true);

	try {
		auto f = ctx.bind<foo>("foo");
		f.constructor<integer*>();
		f.method("t", &foo::t);
		f.prop("x", &foo::x, bind::property_flags::pf_object_pointer);
		f.prop("y", &foo::y, bind::property_flags::pf_none);
		f.prop("z", &foo::z, bind::property_flags::pf_none);
		f.prop("w", &foo::w, bind::property_flags::pf_none);
		f.finalize();

		ctx.bind(print_foo, "print_foo");
	} catch (bind_exception& e) {
		printf("%s\n", e.text.c_str());
	}

	vm_type* tp = ctx.types()->get<foo>();
	ctx.add_code("test.gjs", src);

	std::string last_line = "";
	for (u32 i = 0;i < ctx.code()->size();i++) {
		vm_function* f = ctx.function(i);
		if (f) {
			printf("\n[%s %s(", f->signature.return_type->name.c_str(), f->name.c_str());
			for(u8 a = 0;a < f->signature.arg_types.size();a++) {
				if (a > 0) printf(", ");
				printf("%s arg_%d -> $%s", f->signature.arg_types[a]->name.c_str(), a, register_str[u8(f->signature.arg_locs[a])]);
			}
			printf(")");

			if (f->signature.return_type->size == 0) printf(" -> null");
			else printf(" -> $%s", register_str[u8(f->signature.return_loc)]);
			printf("]\n");
		}
		printf("0x%2.2X: %-32s", i, instruction_to_string((*ctx.code())[i]).c_str());

		auto src = ctx.map()->get(i);
		if (src.lineText != last_line) {
			printf("; %s", src.lineText.c_str());
			last_line = src.lineText;
		}
		printf("\n");
	}
	printf("-------------result-------------\n");

	int x = 5;
	foo test(&x);
	test.y = 10;
	test.z = 4;
	integer result = 0;
	ctx.function("main")->call(&result, &test);
	return 0;
}