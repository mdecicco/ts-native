#include <allocator.h>
#include <context.h>

using namespace gjs;

class foo {
	public:
		foo(int x) { y = x; }
		~foo() { }

		int y;
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
		"integer main() {\n"
		"    return test1(100);\n"
		"}\n";

	vm_allocator* alloc = new basic_malloc_allocator();
	vm_context ctx(alloc, 4096, 4096);
	ctx.log_exceptions(true);

	try {
		bind::wrap_class<foo> f = bind::wrap_class<foo>(ctx.types(), *ctx.jit(), "foo");
		f.constructor<integer>();
		f.prop("y", &foo::y, bind::property_flags::pf_none);
		f.finalize();
	} catch (bind_exception& e) {
		printf("%s\n", e.text.c_str());
	}

	vm_type* tp = ctx.types()->get<foo>();
	ctx.add_code("test.gjs", src);

	//integer result = 0;
	//ctx.function("something")->call(&result, 1, 2, 3.0f);

	//printf("result: %d\n", result);
	return 0;
}