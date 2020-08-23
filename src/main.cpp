#include <allocator.h>
#include <context.h>

using namespace gjs;

class foo {
	public:
		foo(int* _x) { x = _x; }
		~foo() { }

		void t() {
			printf("%d\n", y);
		}

		int* x;
		int y;
		int z;
};

int main(int arg_count, const char** args) {
	const char* src =
		"integer test(integer x);\n"
		"integer something(integer arg0, integer arg1, integer arg2) {\n"
		"    integer x = arg0 + arg1 * 4.0;\n"
		"    integer y = x - arg2;\n"
		"    x = y++;\n"
		"    return test(x);\n"
		"}\n"
		"integer test(integer x) {\n"
		"    return x * 5;\n"
		"}\n"
		"integer test1(integer z) {\n"
		"    return something(z, z + 1, z + 2);\n"
		"}\n"
		"integer main(foo a) {\n"
		"    a.x = 52;\n"
		"    a.y *= 5;\n"
		"    a.z = 2;\n"
		"    a.t();\n"
		"    return test1(100);\n"
		"}\n";

	vm_allocator* alloc = new basic_malloc_allocator();
	vm_context ctx(alloc, 4096, 4096);
	ctx.log_exceptions(true);

	try {
		bind::wrap_class<foo> f = bind::wrap_class<foo>(ctx.types(), *ctx.jit(), "foo");
		f.constructor<integer*>();
		f.method("t", &foo::t);
		f.prop("x", &foo::x, bind::property_flags::pf_object_pointer);
		f.prop("y", &foo::y, bind::property_flags::pf_none);
		f.prop("z", &foo::z, bind::property_flags::pf_none);
		f.finalize();
	} catch (bind_exception& e) {
		printf("%s\n", e.text.c_str());
	}

	vm_type* tp = ctx.types()->get<foo>();
	ctx.add_code("test.gjs", src);

	for (u32 i = 0;i < ctx.code()->size();i++) {
		printf("0x%2.2X: %s\n", i, instruction_to_string((*ctx.code())[i]).c_str());
	}

	int x = 5;
	foo test(&x);
	integer result = 0;
	ctx._function("main")->call(&result, &test);

	//integer result = 0;
	//ctx.function("something")->call(&result, 1, 2, 3.0f);

	//printf("result: %d\n", result);
	return 0;
}