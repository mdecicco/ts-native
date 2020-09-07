#include <compiler/tac.h>
#include <vm_function.h>

namespace gjs {
	namespace compile {
		static const char* op_str[] = {
			"null",
			"load",
			"store",
			"add",
			"sub",
			"mul",
			"div",
			"mod",
			"uadd",
			"usub",
			"umul",
			"udiv",
			"umod",
			"fadd",
			"fsub",
			"fmul",
			"fdiv",
			"fmod",
			"dadd",
			"dsub",
			"dmul",
			"ddiv",
			"dmod",
			"sl",
			"sr",
			"land",
			"lor",
			"band",
			"bor",
			"bxor",
			"ilt",
			"igt",
			"ilte",
			"igte",
			"incmp",
			"icmp",
			"ult",
			"ugt",
			"ulte",
			"ugte",
			"uncmp",
			"ucmp",
			"flt",
			"fgt",
			"flte",
			"fgte",
			"fncmp",
			"fcmp",
			"dlt",
			"dgt",
			"dlte",
			"dgte",
			"dncmp",
			"dcmp",
			"eq",
			"neg",
			"call"
		};

		tac_instruction::tac_instruction(operation _op, const source_ref& _src) : op(_op), src(_src), op_idx(0), callee(nullptr) {
		}

		tac_instruction::~tac_instruction() {
		}

		tac_instruction& tac_instruction::operand(const var& v) {
			if (op_idx == 3) return *this;
			operands[op_idx++] = v;
			return *this;
		}
		
		tac_instruction& tac_instruction::func(vm_function* f) {
			callee = f;
			return *this;
		}

		std::string tac_instruction::to_string() const {
			std::string out = op_str[(u8)op];
			if (callee) return out + " " + callee->name;
			for (u8 i = 0;i < op_idx;i++) out += " " + operands[i].to_string();
			return out;
		}
	};
};