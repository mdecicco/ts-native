#include <instruction_encoder.h>
#include <instruction.h>
#include <register.h>
#include <instruction_array.h>
#include <allocator.h>
#include <value.h>
#include <vm.h>
#include <bind.h>
#include <parse.h>
#include <context.h>
#include <compile.h>

#include <stdio.h>
#include <chrono>

using namespace gjs;

void test_vm() {
	vm_allocator* alloc = new basic_malloc_allocator();
	instruction_array a(alloc);
	using vmi = vm_instruction;
	using vmr = vm_register;

	integer iterations = 100000000;

	// float x = *mem[0xBE]
	// int i = *mem[0xEF]
	// while(i > 0) {
	//	 i--
	//	 x += 10.0f
	// }
	// *mem[0xBE] = x
	vm cpu(alloc, 4096, 4096);
	*(decimal*)cpu.state.memory[0xBE] = 0.0f;
	*(integer*)cpu.state.memory[0xEF] = iterations + 1;
	a += encode(vmi::ld32 ).operand(vmr::s0).operand(vmr::zero).operand(0xBE);  // 0: float x = *mem[0xBE]
	a += encode(vmi::mtfp ).operand(vmr::s0).operand(vmr::f0  );				// 1: ^
	a += encode(vmi::ld32 ).operand(vmr::v2).operand(vmr::zero).operand(0xEF);  // 2: int i = *mem[0xEF]
	a += encode(vmi::subi ).operand(vmr::v2).operand(vmr::v2  ).operand(1	);  // 3: i--
	a += encode(vmi::bneqz).operand(vmr::v2).operand(0x7      );			    // 4: if (i == 0) goto #7
	a += encode(vmi::faddi).operand(vmr::f0).operand(vmr::f0  ).operand(0.1f);  // 5: x += 0.1f
	a += encode(vmi::jmp  ).operand(0x3	   );									// 6: if (i > 0) goto #3
	a += encode(vmi::mffp ).operand(vmr::f0).operand(vmr::s0  );				// 7: *mem[0xBE] = x
	a += encode(vmi::st32 ).operand(vmr::s0).operand(vmr::zero).operand(0xBE);  // 8: ^
	a += encode(vmi::term );													// 9: exit

	std::chrono::high_resolution_clock::time_point tp0 = std::chrono::high_resolution_clock::now();
	cpu.execute(a, 0);
	std::chrono::high_resolution_clock::time_point tp1 = std::chrono::high_resolution_clock::now();
	float tm0 = float(std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp0).count()) / 1000.0f;


	tp0 = std::chrono::high_resolution_clock::now();
	float v = 5.0f;
	integer i = iterations;
	while (i > 0) {
		i--;
		v += 10.0f;
	}
	tp1 = std::chrono::high_resolution_clock::now();
	float tm1 = float(std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp0).count()) / 1000.0f;

	float result = *(float*)cpu.state.memory[0xBE];
	printf("\n\n\n\n\n\n\nResult: %f, in %f seconds (%fx native)\n\n\n\n\n\n", result, tm0, tm0 / tm1);
}

class foo {
	public:
		foo(int x) : y(x) { }
		~foo() { }

		int print() { return printf("%d\n", y); }
		int set_y(int z) { return y = z; }
		int get_y() { return y; }

		int y;
};

void test_bind() {
	asmjit::JitRuntime rt;
	bind::wrapped_function* func = bind::wrap(rt, &foo::print);
	foo bar(15);
	int x = std::any_cast<int>(func->call({ &bar }));
	delete func;

	bind::wrapped_function* construct = bind::wrap_constructor<foo, int>(rt);
	foo* ayy = std::any_cast<foo*>(construct->call({ 52 }));

	bind::wrap_class<foo> f(rt);
	f.constructor<int>();
	f.method("print", &foo::print);
	f.prop("y", &foo::y);
	f.prop("y", &foo::get_y, &foo::set_y);
}

void test_parse() {
	const char* src =
		"format fmt = { a: integer, b: decimal, c: function };\n"
		"void something(fmt arg0, integer arg1);\n"
		"class test {\n"
		"    constructor(string ayy) {\n"
		"        this.x = ayy;\n"
		"        this.y = 10;\n"
		"    }\n"
		"    destructor() { }\n"
		"    void p(decimal x) {\n"
		"        decimal g = x;\n"
		"        for (integer a = 0;a < this.y;a++) {\n"
		"            this.x = this.x + ' ' + this.x;\n"
		"        }\n"
		"    }\n"
		"    \n"
		"    string x;\n"
		"    integer y;\n"
		"};\n"
		"void something(fmt arg0, integer arg1) {\n"
		"    integer x = arg0.a + arg1;\n"
		"}\n";

	vm_context ctx;
	try {
		ast_node* code = parse_source(&ctx, "test.gjs", src);
		code->debug_print(0);
	} catch (parse_exception& e) {
		printf("%s:%d:%d: %s\n", e.file.c_str(), e.line, e.col, e.text.c_str());
		std::string ln = "";
		u32 wscount = 0;
		bool reachedText = false;
		for (u32 i = 0;i < e.lineText.length();i++) {
			if (isspace(e.lineText[i]) && !reachedText) wscount++;
			else {
				reachedText = true;
				ln += e.lineText[i];
			}
		}
		printf("%s\n", ln.c_str());
		for (u32 i = 0;i < e.col - wscount;i++) printf(" ");
		printf("^\n");
	}
}

void test_compile() {
	const char* src =
		"integer test(integer x);\n"
		"integer something(integer arg0, integer arg1, integer arg2) {\n"
		"    integer x = arg0 + arg1 * 4.0;\n"
		"    integer y = x >> arg2;\n"
		"    x = y++;\n"
		"    return test(x);\n"
		"}\n"
		"integer test(integer x) {\n"
		"    return x * 5;\n"
		"}\n"
		"integer test1(integer z) {\n"
		"    return something(z, z + 1, z + 2);\n"
		"}\n";
	vm_context ctx;
	try {
		ast_node* code = parse_source(&ctx, "test.gjs", src);
		code->debug_print(0);
		vm_context ctx;
		vm_allocator* alloc = new basic_malloc_allocator();
		instruction_array bcode = instruction_array(alloc);
		compile_ast(&ctx, code, &bcode);
		for (u32 i = 0;i < bcode.size();i++) {
			printf("%2.2d: %s\n", i, instruction_to_string(bcode[i]).c_str());
			continue;
		}
	} catch (parse_exception& e) {
		printf("%s:%d:%d: %s\n", e.file.c_str(), e.line, e.col, e.text.c_str());
		std::string ln = "";
		u32 wscount = 0;
		bool reachedText = false;
		for (u32 i = 0;i < e.lineText.length();i++) {
			if (isspace(e.lineText[i]) && !reachedText) wscount++;
			else {
				reachedText = true;
				ln += e.lineText[i];
			}
		}
		printf("%s\n", ln.c_str());
		for (u32 i = 0;i < e.col - wscount;i++) printf(" ");
		printf("^\n");
	}
}

int main(int arg_count, const char** args) {
	//test();
	test_compile();
	return 0;
}