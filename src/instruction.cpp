#include <instruction.h>

namespace gjs {
	const char* instruction_str[(i32)vm_instruction::instruction_count] = {
		"null",
		"term",

		// memory
		"ld8",
		"ld16",
		"ld32",
		"ld64",
		"st8",
		"st16",
		"st32",
		"st64",
		"push",
		"pop",
		"mtfp",
		"mffp",

		// arithmetic
		"add",
		"addi",
		"sub",
		"subi",
		"subir",
		"mul",
		"muli",
		"div",
		"divi",
		"divir",

		// integer / floating point conversion
		"ctf",
		"cti",

		// floating point arithmetic
		"fadd",
		"faddi",
		"fsub",
		"fsubi",
		"fsubir",
		"fmul",
		"fmuli",
		"fdiv",
		"fdivi",
		"fdivir",

		// boolean
		"and",
		"andi",
		"or",
		"ori",

		// bitwise
		"band",
		"bandi",
		"bor",
		"bori",
		"xor",
		"xori",
		"sl",
		"sli",
		"sr",
		"sri",

		// control flow
		"beqz",
		"bneqz",
		"bgtz",
		"bgtez",
		"bltz",
		"bltez",
		"jmp",
		"jmpr",
		"jal",
		"jalr"
	};
};
