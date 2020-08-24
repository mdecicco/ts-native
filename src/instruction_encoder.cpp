#include <instruction_encoder.h>
#include <instruction.h>
#include <register.h>
#include <vm_state.h>
#include <parse_utils.h>

namespace gjs {
	using vmi = vm_instruction;
	using vmr = vm_register;

	 #define debug(i) // print_i_bits(i)
	 void print_i_bits (instruction i) {
		system("cls");
		for (int b = 63;b >= 0;b--) printf("%2.2d: %d\n", 64 - b, (u8)(i >> b) & 1);
	 }
	
	instruction_encoder::instruction_encoder(vm_instruction i) {
		m_result = instruction(i) << instruction_shift;
		debug(m_result);
	}

	// term, null
	#define check_instr_type_0(x)	\
		(							\
			   x == vmi::term		\
			|| x == vmi::null		\
		)

	// jal, jmp
	#define check_instr_type_1(x)	\
		(							\
		       x == vmi::jal		\
			|| x == vmi::jmp		\
		)

	// jalr, jmpr, push, pop, ctf, cti
	#define check_instr_type_2(x)	\
		(							\
			   x == vmi::jalr		\
			|| x == vmi::jmpr		\
			|| x == vmi::ctf		\
			|| x == vmi::cti		\
			|| x == vmi::push		\
			|| x == vmi::pop		\
		)

	// b*, push, pop
	#define check_instr_type_3(x)	\
		(							\
			   x == vmi::beqz		\
			|| x == vmi::bneqz		\
			|| x == vmi::bgtz		\
			|| x == vmi::bgtez		\
			|| x == vmi::bltz		\
			|| x == vmi::bltez		\
		)

	// mtfp, mffp
	#define check_instr_type_4(x)	\
		(							\
		       x == vmi::mtfp		\
			|| x == vmi::mffp		\
		)

	// ld*, st*
	#define check_instr_type_5(x)	\
		(							\
		       x == vmi::ld8		\
			|| x == vmi::ld16		\
			|| x == vmi::ld32		\
			|| x == vmi::ld64		\
			|| x == vmi::st8		\
			|| x == vmi::st16		\
			|| x == vmi::st32		\
			|| x == vmi::st64		\
		)

	// *i, *ir

	#define check_instr_type_6(x)	\
		(							\
			   x == vmi::addi		\
			|| x == vmi::subi		\
			|| x == vmi::subir		\
			|| x == vmi::muli		\
			|| x == vmi::divi		\
			|| x == vmi::divir		\
			|| x == vmi::addui		\
			|| x == vmi::subui		\
			|| x == vmi::subuir		\
			|| x == vmi::mului		\
			|| x == vmi::divui		\
			|| x == vmi::divuir		\
			|| x == vmi::faddi		\
			|| x == vmi::fsubi		\
			|| x == vmi::fsubir		\
			|| x == vmi::fmuli		\
			|| x == vmi::fdivi		\
			|| x == vmi::fdivir		\
			|| x == vmi::bandi		\
			|| x == vmi::bori		\
			|| x == vmi::xori		\
			|| x == vmi::sli		\
			|| x == vmi::sri		\
			|| x == vmi::andi		\
			|| x == vmi::ori		\
			|| x == vmi::lti		\
			|| x == vmi::ltei		\
			|| x == vmi::gti		\
			|| x == vmi::gtei		\
			|| x == vmi::cmpi		\
			|| x == vmi::ncmpi		\
			|| x == vmi::flti		\
			|| x == vmi::fltei		\
			|| x == vmi::fgti		\
			|| x == vmi::fgtei		\
			|| x == vmi::fcmpi		\
			|| x == vmi::fncmpi		\
		)

	// the rest
	#define check_instr_type_7(x)	\
		(							\
			   x == vmi::add		\
			|| x == vmi::sub		\
			|| x == vmi::mul		\
			|| x == vmi::div		\
			|| x == vmi::addu		\
			|| x == vmi::subu		\
			|| x == vmi::mulu		\
			|| x == vmi::divu		\
			|| x == vmi::fadd		\
			|| x == vmi::fsub		\
			|| x == vmi::fmul		\
			|| x == vmi::fdiv		\
			|| x == vmi::and		\
			|| x == vmi::or			\
			|| x == vmi::band		\
			|| x == vmi::bor		\
			|| x == vmi::xor		\
			|| x == vmi::sl			\
			|| x == vmi::sr			\
			|| x == vmi::lt			\
			|| x == vmi::lte		\
			|| x == vmi::gt			\
			|| x == vmi::gte		\
			|| x == vmi::cmp		\
			|| x == vmi::ncmp		\
			|| x == vmi::flt		\
			|| x == vmi::flte		\
			|| x == vmi::fgt		\
			|| x == vmi::fgte		\
			|| x == vmi::fcmp		\
			|| x == vmi::fncmp		\
		)

	#define first_operand_is_register(x) \
		(								 \
			   !check_instr_type_0(x)	 \
			&& !check_instr_type_1(x)	 \
		)

	#define first_operand_is_immediate(x) \
		check_instr_type_1(x)

	#define second_operand_is_register(x)	\
		(									\
			   !check_instr_type_0(x)		\
			&& !check_instr_type_1(x)		\
			&& !check_instr_type_2(x)		\
			&& !check_instr_type_3(x)		\
		)

	#define second_operand_is_immediate(x) \
		check_instr_type_3(x)

	#define third_operand_is_register(x) \
		check_instr_type_7(x)

	#define third_operand_is_immediate(x)	\
		(									\
			   check_instr_type_5(x)		\
			|| check_instr_type_6(x)		\
		)

	#define third_operand_can_be_float(x) \
		check_instr_type_6(x)
		
	#define first_operand_must_be_fpr(x) \
		(								 \
			   x == vmi::mffp			 \
			|| x == vmi::ctf			 \
			|| x == vmi::cti			 \
			|| x == vmi::fadd			 \
			|| x == vmi::fsub			 \
			|| x == vmi::fmul			 \
			|| x == vmi::fdiv			 \
			|| x == vmi::faddi			 \
			|| x == vmi::fsubi			 \
			|| x == vmi::fsubir			 \
			|| x == vmi::fmuli			 \
			|| x == vmi::fdivi			 \
			|| x == vmi::fdivir			 \
		)
	
	#define second_operand_must_be_fpr(x) \
		(								  \
			   x == vmi::mtfp			  \
			|| x == vmi::fadd			  \
			|| x == vmi::fsub			  \
			|| x == vmi::fmul			  \
			|| x == vmi::fdiv			  \
			|| x == vmi::faddi			  \
			|| x == vmi::fsubi			  \
			|| x == vmi::fsubir			  \
			|| x == vmi::fmuli			  \
			|| x == vmi::fdivi			  \
			|| x == vmi::fdivir			  \
			|| x == vmi::flti			  \
			|| x == vmi::fltei			  \
			|| x == vmi::fgti			  \
			|| x == vmi::fgtei			  \
			|| x == vmi::fcmpi			  \
			|| x == vmi::fncmpi			  \
			|| x == vmi::flt			  \
			|| x == vmi::flte			  \
			|| x == vmi::fgt			  \
			|| x == vmi::fgte			  \
			|| x == vmi::fcmp			  \
			|| x == vmi::fncmp			  \
		)
	
	#define third_operand_must_be_fpr(x)  \
		(								  \
			   x == vmi::fadd			  \
			|| x == vmi::fsub			  \
			|| x == vmi::fmul			  \
			|| x == vmi::fdiv			  \
			|| x == vmi::fncmpi			  \
			|| x == vmi::flt			  \
			|| x == vmi::flte			  \
			|| x == vmi::fgt			  \
			|| x == vmi::fgte			  \
			|| x == vmi::fcmp			  \
			|| x == vmi::fncmp			  \
		)
	
	#define third_operand_must_be_fpi(x) \
		(								 \
			   x == vmi::faddi			 \
			|| x == vmi::fsubi			 \
			|| x == vmi::fsubir			 \
			|| x == vmi::fmuli			 \
			|| x == vmi::fdivi			 \
			|| x == vmi::fdivir			 \
			|| x == vmi::flti			 \
			|| x == vmi::fltei			 \
			|| x == vmi::fgti			 \
			|| x == vmi::fgtei			 \
			|| x == vmi::fcmpi			 \
			|| x == vmi::fncmpi			 \
		)

	#define is_fpr(x) (x >= vmr::f0 && x <= vmr::f15)

	instruction_encoder& instruction_encoder::operand(vm_register reg) {
		vmi instr = decode_instruction(m_result);
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (check_instr_type_1(instr)) {
			// instruction takes a 32 bit integer as the only operand
			// exception
			return *this;
		}

		if (m_result & (1 << 2)) {
			// operand 3 already set, no instructions take a 4th operand
			// exception
			return *this;
		}

		// maybe set operand 3
		if (m_result & (1 << 1)) {
			// operand 2 already set
			if (!third_operand_is_register(instr)) {
				// instruction does not need third operand or third operand can not be a register
				// exception
				return *this;
			}

			if (third_operand_must_be_fpr(instr) != (is_fpr(reg) || reg == vmr::zero)) {
				// invalid operand
				// exception
				return *this;
			}

			m_result |= 1 << 2;
			m_result |= instruction(reg) << operand_3r_shift;
			debug(m_result);
			return *this;
		}
		
		// maybe set operand 2
		if (m_result & 1) {
			// operand 1 already set
			if (!second_operand_is_register(instr)) {
				// instruction does not need second operand or second operand can not be a register
				// exception
				return *this;
			}

			if (second_operand_must_be_fpr(instr) != (is_fpr(reg) || reg == vmr::zero)) {
				// invalid operand
				// exception
				return *this;
			}

			m_result |= 1 << 1;
			m_result |= instruction(reg) << operand_2r_shift;
			debug(m_result);
			return *this;
		}

		// operand 1
		if (!first_operand_is_register(instr)) {
			// instruction does not need an operand or first operand can not be a register
			// exception
			return *this;
		}

		if (first_operand_must_be_fpr(instr) != is_fpr(reg) && !(check_instr_type_5(instr) && is_fpr(reg))) {
			// insnstruction requires operand 1 to be floating point register
			// exception
			return *this;
		}

		m_result |= 1;
		m_result |= instruction(reg) << operand_1r_shift;
		debug(m_result);
		return *this;
	}

	instruction_encoder& instruction_encoder::operand(integer immediate) {
		vmi instr = decode_instruction(m_result);
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (m_result & (1 << 2)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (m_result & (1 << 1)) {
			// operand 2 already set
			if (!third_operand_is_immediate(instr)) {
				// instruction does not take third operand or third operand is not immediate
				// exception
				return *this;
			}

			if (third_operand_must_be_fpi(instr)) {
				// instruction requires that the third operand be floating point
				// exception
				return *this;
			}

			m_result |= 1 << 2;
			m_result |= instruction(immediate) << operand_3i_shift;
			debug(m_result);
			return *this;
		}

		if (m_result & 1) {
			// operand 1 already set
			if (!second_operand_is_immediate(instr)) {
				// instruction does not take second operand or second operand is not immediate
				// exception
				return *this;
			}

			m_result |= 1 << 1;
			m_result |= instruction(immediate) << operand_2i_shift;
			debug(m_result);
			return *this;
		}

		// operand 1
		if (!first_operand_is_immediate(instr)) {
			// instruction does not take an operand or first operand is not immediate
			// exception
			return *this;
		}
		m_result |= 1;
		m_result |= instruction(immediate) << operand_1i_shift;
		debug(m_result);
		return *this;
	}

	instruction_encoder& instruction_encoder::operand(uinteger immediate) {
		vmi instr = decode_instruction(m_result);
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (m_result & (1 << 2)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (m_result & (1 << 1)) {
			// operand 2 already set
			if (!third_operand_is_immediate(instr)) {
				// instruction does not take third operand or third operand is not immediate
				// exception
				return *this;
			}

			if (third_operand_must_be_fpi(instr)) {
				// instruction requires that the third operand be floating point
				// exception
				return *this;
			}

			m_result |= 1 << 2;
			m_result |= instruction(immediate) << operand_3i_shift;
			debug(m_result);
			return *this;
		}

		if (m_result & 1) {
			// operand 1 already set
			if (!second_operand_is_immediate(instr)) {
				// instruction does not take second operand or second operand is not immediate
				// exception
				return *this;
			}

			m_result |= 1 << 1;
			m_result |= instruction(immediate) << operand_2i_shift;
			debug(m_result);
			return *this;
		}

		// operand 1
		if (!first_operand_is_immediate(instr)) {
			// instruction does not take an operand or first operand is not immediate
			// exception
			return *this;
		}
		m_result |= 1;
		m_result |= instruction(immediate) << operand_1i_shift;
		debug(m_result);
		return *this;
	}

	instruction_encoder& instruction_encoder::operand(decimal immediate) {
		vmi instr = decode_instruction(m_result);
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (m_result & (1 << 2)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (m_result & (1 << 1)) {
			// operand 2 already set
			if (!third_operand_is_immediate(instr)) {
				// instruction does not take third operand or third operand is not immediate
				// exception
				return *this;
			}

			if (!third_operand_can_be_float(instr)) {
				// instruction does not take third operand that can be a float
				// exception
				return *this;
			}

			debug(m_result);
			m_result |= 1 << 2;
			debug(m_result);
			m_result |= instruction(1) << operand_3_is_flt_shift;
			debug(m_result);
			m_result |= instruction(*(integer*)&immediate) << operand_3i_shift;
			debug(m_result);
			return *this;
		}

		// only third operand can possibly be a float
		// exception
		return *this;
	}

	instruction_encoder& instruction_encoder::operand(u64 immediate) {
		vmi instr = decode_instruction(m_result);
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (!check_instr_type_1(instr)) {
			// only type 1 instructions can take this kind of operand
			// exception
			return *this;
		}

		if (m_result & (1 << 2)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (m_result & (1 << 1)) {
			// operand 2 already set
			// this type of operand can only be assigned to the first operand
			// exception
			return *this;
		}

		if (m_result & 1) {
			// operand 1 already set
			// this type of operand can only be assigned to the first operand
			// exception
			return *this;
		}

		// operand 1
		if (!first_operand_is_immediate(instr)) {
			// instruction does not take an operand or first operand is not immediate
			// exception
			return *this;
		}

		m_result |= instruction((immediate | operand_1_integer_mask) ^ operand_1_integer_mask) << operand_1i_shift;
		debug(m_result);
		return *this;
	}

	std::string instruction_to_string(instruction code, vm_state* state) {
		std::string out;
		vmi i = decode_instruction(code);
		if (i >= vmi::instruction_count) return "Invalid Instruction";

		if (check_instr_type_0(i)) return instruction_str[(u8)i];

		auto reg_str = [state, i](vmr r, i32 offset = 0, bool is_mem = false) {
			if (!state) return "$" + std::string(register_str[(integer)r]);
			else {
				/*
				vm_memory* mem = nullptr;
				if (check_instr_type_5(i) && is_mem) mem = &state->memory;

				if (mem) {
					std::string val = (*mem)[(integer)state->registers[(integer)r] + offset].to_string();
					return "$" + std::string(register_str[(integer)r]) + "<" + val + ">";
				}
				*/

				std::string reg_val = ""; // "<" + state->registers[(integer)r].to_string() + ">"

				if (is_fpr(r)) reg_val = format("<%f>", decimal(state->registers[(integer)r]));
				else reg_val = format("<%d>", integer(state->registers[(integer)r]));
				return "$" + std::string(register_str[(integer)r]) + reg_val;
			}

			return std::string();
		};

		out += instruction_str[(u8)i];
		out += ' ';
		if (check_instr_type_1(i)) {
			u64 o1 = decode_operand_1ui64(code);
			char addr[64] = { 0 };
			_itoa_s<64>(o1, addr, 16);
			out += "0x";
			out += addr;
		} else if (check_instr_type_2(i)) {
			vmr o1 = decode_operand_1(code);
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
		} else if (check_instr_type_3(i)) {
			vmr o1 = decode_operand_1(code);
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			integer o2 = decode_operand_2i(code);
			char addr[32] = { 0 };
			_itoa_s<32>(o2, addr, 16);
			out += "0x";
			out += addr;
		} else if (check_instr_type_5(i)) {
			vmr o1 = decode_operand_1(code);
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = decode_operand_2(code);
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			integer o3 = decode_operand_3i(code);
			char offset[32] = { 0 };
			_itoa_s<32>(o3, offset, 10);
			out += offset;
			out += '(' + reg_str(o2, o3, true) + ')';
		} else if (check_instr_type_6(i)) {
			vmr o1 = decode_operand_1(code);
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = decode_operand_2(code);
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o2);
			out += ", ";

			if (decode_operator_3_is_float(code)) {
				decimal o3 = decode_operand_3f(code);
				char imm[32] = { 0 };
				snprintf(imm, 32, "%f", o3);
				out += imm;
			} else {
				integer o3 = decode_operand_3i(code);
				char imm[32] = { 0 };
				_itoa_s<32>(o3, imm, 10);
				out += imm;
			}

		} else if (check_instr_type_7(i)) {
			vmr o1 = decode_operand_1(code);
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = decode_operand_2(code);
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o2);
			out += ", ";

			vmr o3 = decode_operand_3(code);
			if (o3 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o3);
		}

		return out;
	}
};