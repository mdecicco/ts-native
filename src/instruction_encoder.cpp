#include <instruction.h>
#include <instruction_encoder.h>
#include <register.h>
#include <context.h>
#include <util.h>

namespace gjs {
	using vmi = vm_instruction;
	using vmr = vm_register;

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

	// jalr, jmpr, push, pop, cvt_*
	#define check_instr_type_2(x)	\
		(							\
			   x == vmi::jalr		\
			|| x == vmi::jmpr		\
			|| x == vmi::cvt_if		\
			|| x == vmi::cvt_id		\
			|| x == vmi::cvt_uf		\
			|| x == vmi::cvt_ud		\
			|| x == vmi::cvt_fi		\
			|| x == vmi::cvt_fu		\
			|| x == vmi::cvt_fd		\
			|| x == vmi::cvt_di		\
			|| x == vmi::cvt_du		\
			|| x == vmi::cvt_df		\
			|| x == vmi::push		\
			|| x == vmi::pop		\
		)

	// bits*, push, pop
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
			|| x == vmi::lti		\
			|| x == vmi::ltei		\
			|| x == vmi::gti		\
			|| x == vmi::gtei		\
			|| x == vmi::cmpi		\
			|| x == vmi::ncmpi		\
			|| x == vmi::faddi		\
			|| x == vmi::fsubi		\
			|| x == vmi::fsubir		\
			|| x == vmi::fmuli		\
			|| x == vmi::fdivi		\
			|| x == vmi::fdivir		\
			|| x == vmi::flti		\
			|| x == vmi::fltei		\
			|| x == vmi::fgti		\
			|| x == vmi::fgtei		\
			|| x == vmi::fcmpi		\
			|| x == vmi::fncmpi		\
			|| x == vmi::daddi		\
			|| x == vmi::dsubi		\
			|| x == vmi::dsubir		\
			|| x == vmi::dmuli		\
			|| x == vmi::ddivi		\
			|| x == vmi::ddivir		\
			|| x == vmi::dlti		\
			|| x == vmi::dltei		\
			|| x == vmi::dgti		\
			|| x == vmi::dgtei		\
			|| x == vmi::dcmpi		\
			|| x == vmi::dncmpi		\
			|| x == vmi::bandi		\
			|| x == vmi::bori		\
			|| x == vmi::xori		\
			|| x == vmi::sli		\
			|| x == vmi::slir		\
			|| x == vmi::sri		\
			|| x == vmi::srir		\
			|| x == vmi::andi		\
			|| x == vmi::ori		\
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
			|| x == vmi::lt			\
			|| x == vmi::lte		\
			|| x == vmi::gt			\
			|| x == vmi::gte		\
			|| x == vmi::cmp		\
			|| x == vmi::ncmp		\
			|| x == vmi::fadd		\
			|| x == vmi::fsub		\
			|| x == vmi::fmul		\
			|| x == vmi::fdiv		\
			|| x == vmi::flt		\
			|| x == vmi::flte		\
			|| x == vmi::fgt		\
			|| x == vmi::fgte		\
			|| x == vmi::fcmp		\
			|| x == vmi::fncmp		\
			|| x == vmi::dadd		\
			|| x == vmi::dsub		\
			|| x == vmi::dmul		\
			|| x == vmi::ddiv		\
			|| x == vmi::dlt		\
			|| x == vmi::dlte		\
			|| x == vmi::dgt		\
			|| x == vmi::dgte		\
			|| x == vmi::dcmp		\
			|| x == vmi::dncmp		\
			|| x == vmi::and		\
			|| x == vmi::or			\
			|| x == vmi::band		\
			|| x == vmi::bor		\
			|| x == vmi::xor		\
			|| x == vmi::sl			\
			|| x == vmi::sr			\
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
			|| x == vmi::flti			  \
			|| x == vmi::fltei			  \
			|| x == vmi::fgti			  \
			|| x == vmi::fgtei			  \
			|| x == vmi::fcmpi			  \
			|| x == vmi::fncmpi			  \
			|| x == vmi::dadd			  \
			|| x == vmi::dsub			  \
			|| x == vmi::dmul			  \
			|| x == vmi::ddiv			  \
			|| x == vmi::daddi			  \
			|| x == vmi::dsubi			  \
			|| x == vmi::dsubir			  \
			|| x == vmi::dmuli			  \
			|| x == vmi::ddivi			  \
			|| x == vmi::ddivir			  \
			|| x == vmi::dncmpi			  \
			|| x == vmi::dlti			  \
			|| x == vmi::dltei			  \
			|| x == vmi::dgti			  \
			|| x == vmi::dgtei			  \
			|| x == vmi::dcmpi			  \
			|| x == vmi::dncmpi			  \
		)

	// push, pop, cvt_*
	#define first_operand_can_be_fpr(x)	\
		(								\
		       x == vmi::ld8			\
			|| x == vmi::ld16			\
			|| x == vmi::ld32			\
			|| x == vmi::ld64			\
			|| x == vmi::st8			\
			|| x == vmi::st16			\
			|| x == vmi::st32			\
			|| x == vmi::st64			\
			|| x == vmi::cvt_if			\
			|| x == vmi::cvt_id			\
			|| x == vmi::cvt_uf			\
			|| x == vmi::cvt_ud			\
			|| x == vmi::cvt_fi			\
			|| x == vmi::cvt_fu			\
			|| x == vmi::cvt_fd			\
			|| x == vmi::cvt_di			\
			|| x == vmi::cvt_du			\
			|| x == vmi::cvt_df			\
			|| x == vmi::push			\
			|| x == vmi::pop			\
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
			|| x == vmi::dadd			  \
			|| x == vmi::dsub			  \
			|| x == vmi::dmul			  \
			|| x == vmi::ddiv			  \
			|| x == vmi::daddi			  \
			|| x == vmi::dsubi			  \
			|| x == vmi::dsubir			  \
			|| x == vmi::dmuli			  \
			|| x == vmi::ddivi			  \
			|| x == vmi::ddivir			  \
			|| x == vmi::dlti			  \
			|| x == vmi::dltei			  \
			|| x == vmi::dgti			  \
			|| x == vmi::dgtei			  \
			|| x == vmi::dcmpi			  \
			|| x == vmi::dncmpi			  \
			|| x == vmi::dlt			  \
			|| x == vmi::dlte			  \
			|| x == vmi::dgt			  \
			|| x == vmi::dgte			  \
			|| x == vmi::dcmp			  \
			|| x == vmi::dncmp			  \
		)
	
	#define third_operand_must_be_fpr(x)  \
		(								  \
			   x == vmi::fadd			  \
			|| x == vmi::fsub			  \
			|| x == vmi::fmul			  \
			|| x == vmi::fdiv			  \
			|| x == vmi::flt			  \
			|| x == vmi::flte			  \
			|| x == vmi::fgt			  \
			|| x == vmi::fgte			  \
			|| x == vmi::fcmp			  \
			|| x == vmi::fncmp			  \
			|| x == vmi::dadd			  \
			|| x == vmi::dsub			  \
			|| x == vmi::dmul			  \
			|| x == vmi::ddiv			  \
			|| x == vmi::dlt			  \
			|| x == vmi::dlte			  \
			|| x == vmi::dgt			  \
			|| x == vmi::dgte			  \
			|| x == vmi::dcmp			  \
			|| x == vmi::dncmp			  \
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
			|| x == vmi::daddi			 \
			|| x == vmi::dsubi			 \
			|| x == vmi::dsubir			 \
			|| x == vmi::dmuli			 \
			|| x == vmi::ddivi			 \
			|| x == vmi::ddivir			 \
			|| x == vmi::dncmpi			 \
			|| x == vmi::dlti			 \
			|| x == vmi::dltei			 \
			|| x == vmi::dgti			 \
			|| x == vmi::dgtei			 \
			|| x == vmi::dcmpi			 \
			|| x == vmi::dncmpi			 \
		)

	#define is_fpr(x) (x >= vmr::f0 && x <= vmr::f15)
	#define decode_instr ((vmi)(m_code >> instr_shift))
	#define check_flag(f) (((m_code | flag_mask) ^ flag_mask) & f)
	#define set_flag(f) (m_code |= f)

	instruction::instruction() : m_code(0), m_imm(0) { }

	instruction::instruction(vm_instruction i) : m_code(0), m_imm(0) {
		m_code |= u32(i) << instr_shift;
	}

	instruction& instruction::operand(vm_register reg) {
		vmi instr = decode_instr;
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

		if (check_flag(op_3_assigned)) {
			// operand 3 already set, no instructions take a 4th operand
			// exception
			return *this;
		}

		// maybe set operand 3
		if (check_flag(op_2_assigned)) {
			// operand 2 already set
			if (!third_operand_is_register(instr)) {
				// instruction does not need third operand or third operand can not be a register
				// exception
				return *this;
			}

			if ((third_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!third_operand_must_be_fpr(instr) && is_fpr(reg))) {
				// invalid operand
				// exception
				return *this;
			}

			set_flag(op_3_assigned);
			m_code |= u32(reg) << op_3_shift;
			return *this;
		}
		
		// maybe set operand 2
		if (check_flag(op_1_assigned)) {
			// operand 1 already set
			if (!second_operand_is_register(instr)) {
				// instruction does not need second operand or second operand can not be a register
				// exception
				return *this;
			}

			if ((second_operand_must_be_fpr(instr) && !(is_fpr(reg) || reg == vmr::zero)) || (!second_operand_must_be_fpr(instr) && is_fpr(reg))) {
				// invalid operand
				// exception
				return *this;
			}

			set_flag(op_2_assigned);
			m_code |= u32(reg) << op_2_shift;
			return *this;
		}

		// operand 1
		if (!first_operand_is_register(instr)) {
			// instruction does not need an operand or first operand can not be a register
			// exception
			return *this;
		}

		if (first_operand_must_be_fpr(instr) != is_fpr(reg) && !(first_operand_can_be_fpr(instr) && is_fpr(reg))) {
			// insnstruction requires operand 1 to be floating point register
			// exception
			return *this;
		}

		set_flag(op_1_assigned);
		m_code |= u32(reg) << op_1_shift;
		return *this;
	}

	instruction& instruction::operand(i64 immediate) {
		return operand(*(u64*)&immediate);
	}

	instruction& instruction::operand(u64 immediate) {
		vmi instr = decode_instr;
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (check_flag(op_3_assigned)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (check_flag(op_2_assigned)) {
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

			set_flag(op_3_assigned);
			m_imm = immediate;
			return *this;
		}

		if (check_flag(op_1_assigned)) {
			// operand 1 already set
			if (!second_operand_is_immediate(instr)) {
				// instruction does not take second operand or second operand is not immediate
				// exception
				return *this;
			}

			set_flag(op_2_assigned);
			m_imm = immediate;
			return *this;
		}

		// operand 1
		if (!first_operand_is_immediate(instr)) {
			// instruction does not take an operand or first operand is not immediate
			// exception
			return *this;
		}
		
		set_flag(op_1_assigned);
		return *this;
	}

	instruction& instruction::operand(f64 immediate) {
		vmi instr = decode_instr;
		if (check_instr_type_0(instr)) {
			// instruction takes no operands
			// exception
			return *this;
		}

		if (check_flag(op_3_assigned)) {
			// operand 3 already set
			// exception
			return *this;
		}

		if (check_flag(op_2_assigned)) {
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

			set_flag(op_3_is_float);
			m_imm = *(u64*)&immediate;
			return *this;
		}

		// only third operand can possibly be a float
		// exception
		return *this;
	}

	std::string instruction::to_string(vm_context* ctx) const {
		vm_state* state = ctx ? ctx->state() : nullptr;

		std::string out;
		vmi i = instr();
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

				if (is_fpr(r)) reg_val = format("<%f>", *(f32*)&state->registers[(integer)r]);
				else reg_val = format("<%d>", *(i32*)&state->registers[(integer)r]);
				return "$" + std::string(register_str[(integer)r]) + reg_val;
			}

			return std::string();
		};

		out += instruction_str[(u8)i];
		while (out.length() < 8) out += ' ';
		if (check_instr_type_1(i)) {
			u64 o1 = imm_u();
			out += format("0x%llX", o1);
		} else if (check_instr_type_2(i)) {
			vmr o1 = op_1r();
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
		} else if (check_instr_type_3(i)) {
			vmr o1 = op_1r();
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			integer o2 = imm_u();
			out += format("0x%llX", o2);
		} else if (check_instr_type_5(i)) {
			vmr o1 = op_1r();
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = op_2r();
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			integer o3 = imm_i();
			char offset[32] = { 0 };
			_itoa_s<32>(o3, offset, 10);
			out += offset;
			out += '(' + reg_str(o2, o3, true) + ')';
		} else if (check_instr_type_6(i)) {
			vmr o1 = op_1r();
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = op_2r();
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o2);
			out += ", ";

			if (imm_is_flt()) {
				decimal o3 = imm_f();
				char imm[32] = { 0 };
				snprintf(imm, 32, "%f", o3);
				out += imm;
			} else {
				integer o3 = imm_i();
				char imm[32] = { 0 };
				_itoa_s<32>(o3, imm, 10);
				out += imm;
			}

		} else if (check_instr_type_7(i)) {
			vmr o1 = op_1r();
			if (o1 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o1);
			out += ", ";

			vmr o2 = op_2r();
			if (o2 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o2);
			out += ", ";

			vmr o3 = op_3r();
			if (o3 >= vmr::register_count) return "Invalid Instruction";

			out += reg_str(o3);
		}

		return out;
	}
};