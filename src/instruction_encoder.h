#pragma once
#include <types.h>
#include <string>

namespace gjs {
	enum class vm_instruction;
	enum class vm_register;

	/*
	 * Anatomy of an encoded instruction
	 *
	 * type 0  | instruction |-----------------------------------------------------------------------------------------------------------------| <- term, null
	 * type 1  | instruction |                      32-bit jump address                      |-------------------------------------------------| <- jal, jmp
	 * type 2  | instruction |  operand  |-----------------------------------------------------------------------------------------------------| <- jalr, jmpr, ctf, cti, push, pop
	 * type 3  | instruction |  operand  |             32-bit branch failure address                     |-------------------------------------| <- b*
	 * type 4  | instruction |  operand  |  operand  |-----------------------------------------------------------------------------------------| <- mtfp, mffp
	 * type 5  | instruction |  operand  |  operand  |x|                      32-bit immediate value                   |-----------------------| <- ld*, st*
	 * type 6  | instruction |  operand  |  operand  |x|                      32-bit immediate value                   |-----------------------| <- *i, *ir
	 * type 7  | instruction |  operand  |  operand  |  operand  |-----------------------------------------------------------------------------| <- the rest
	 *         |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0| (bits)
	 *         |               |               |               |               |               |               |               |               | (bytes)
	 *                       ^ 57        ^ 51        ^ 45        ^ 39                        ^ 25        | ^18         ^ 12
	 *	x = immediate value is float32 (boolean)
	 */

	class instruction_encoder {
		public:
			instruction_encoder(vm_instruction i);
			~instruction_encoder() { }

			instruction_encoder& operand(vm_register reg);
			instruction_encoder& operand(integer immediate);
			instruction_encoder& operand(uinteger immediate);
			instruction_encoder& operand(decimal immediate);

			inline operator instruction() const { return m_result; }

		protected:
			instruction m_result;
	};

	inline instruction_encoder encode(vm_instruction i) { return instruction_encoder(i); }

	const u64 instruction_shift = 57;
	const u64 operand_1r_shift = 51;
	const u64 operand_1i_shift = 25;
	const u64 operand_2r_shift = 45;
	const u64 operand_2i_shift = 18;
	const u64 operand_3r_shift = 39;
	const u64 operand_3i_shift = 12;
	const u64 operand_3_is_flt_shift = 44;
	const u64 operand_register_mask = 0b1111111111111111111111111111111111111111111111111111111111000000UL;


	// the following have undefined results if the instruction does not support the item being decoded
	FORCE_INLINE vm_instruction decode_instruction			(instruction i) { return (vm_instruction)(i >> instruction_shift); }
	FORCE_INLINE vm_register	decode_operand_1			(instruction i) { return (vm_register)(((i >> operand_1r_shift) | operand_register_mask) ^ operand_register_mask); }
	FORCE_INLINE integer		decode_operand_1i			(instruction i) { return i >> operand_1i_shift; }
	FORCE_INLINE uinteger		decode_operand_1ui			(instruction i) { return i >> operand_1i_shift; }
	FORCE_INLINE vm_register	decode_operand_2			(instruction i) { return (vm_register)(((i >> operand_2r_shift) | operand_register_mask) ^ operand_register_mask); }
	FORCE_INLINE integer		decode_operand_2i			(instruction i) { return i >> operand_2i_shift; }
	FORCE_INLINE uinteger		decode_operand_2ui			(instruction i) { return i >> operand_2i_shift; }
	FORCE_INLINE vm_register	decode_operand_3			(instruction i) { return (vm_register)(((i >> operand_3r_shift) | operand_register_mask) ^ operand_register_mask); }
	FORCE_INLINE bool			decode_operator_3_is_float	(instruction i) { return (i >> operand_3_is_flt_shift) & 1; }
	FORCE_INLINE decimal		decode_operand_3f			(instruction i) { float r; *((integer*)&r) = i >> operand_3i_shift; return r; }
	FORCE_INLINE integer		decode_operand_3i			(instruction i) { return i >> operand_3i_shift; }
	FORCE_INLINE uinteger		decode_operand_3ui			(instruction i) { return i >> operand_3i_shift; }

	class vm_state;
	std::string instruction_to_string(instruction i, vm_state* state = nullptr);
};

