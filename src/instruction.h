#pragma once
#include <types.h>

namespace gjs {
	// number of bits needed to store instruction id: 7
	// max number of instructions 127
	enum class vm_instruction {
		null		= 0 ,	// do nothing
		term			,	// terminate execution

		// load src/store dest can have an offset, and load from/store in the memory pointed to
		// by (src) + offset / (dest) + offset.

		// floating point registers can only be used by the floating point arithmetic instructions,
		// the instructions that move data to/from them, and the instructions that convert values
		// in them between integer and floating point before/after moving from/to a gp register

		// memory moving
		ld8			    ,	// load 1 byte from memory into register			ld8		(dest)	(src)	0		dest = *((u8*)(src + 0))
		ld16		    ,	// load 2 bytes from memory into register			ld16	(dest)	(src)	0		dest = *((u16*)(src + 0))
		ld32		    ,	// load 4 bytes from memory into register			ld32	(dest)	(src)	0		dest = *((u32*)(src + 0))
		ld64		    ,	// load 8 bytes from memory into register			ld64	(dest)	(src)	0		dest = *((u64*)(src + 0))
		st8				,	// store 1 byte in memory from register				store	(src)	(dest)	0xa		dest = *((u8*)(src + 0xa))
		st16			,	// store 2 bytes in memory from register			store	(src)	(dest)	0xa		dest = *((u16*)(src + 0xa))
		st32			,	// store 4 bytes in memory from register			store	(src)	(dest)	0xa		dest = *((u32*)(src + 0xa))
		st64			,	// store 8 bytes in memory from register			store	(src)	(dest)	0xa		dest = *((u64*)(src + 0xa))
		push			,	// push register onto stack							push	(a)						*sp = a; sp--
		pop				,	// pop value from the stack into register			pop		(a)						a = *sp; sp++
		mtfp			,	// move from general register to float register		mtfp	(a)		(b)				b = a				(a must be gp, b must be fp)
		mffp			,	// move from float register to general register		mffp	(a)		(b)				b =	a				(a must be fp, b must be gp)

		// integer arithmetic
		add				,	// add two registers								add		(dest)	(a)		(b)		dest = a + b
		addi			,	// add register and immediate value					addi	(dest)	(a)		1.0		dest = a + 1
		sub				,	// subtract register from another					sub		(dest)	(a)		(b)		dest = a - b
		subi			,	// subtract immediate value from register			subi	(dest)	(a)		1.0		dest = a - 1
		subir			,	// subtract register from immediate value			subir	(dest)	(a)		1.0		dest = 1 - a
		mul				,	// multiply two registers							mul		(dest)	(a)		(b)		dest = a * b
		muli			,	// multiply register and immediate value			muli	(dest)	(a)		1.0		dest = a * 1
		div				,	// divide register by another						div		(dest)	(a)		(b)		dest = a / b
		divi			,	// divide register by immediate value				divi	(dest)	(a)		1.0		dest = a / 1
		divir			,	// divide immediate value by register				divir	(dest)	(a)		1.0		dest = 1 / a

		// unsigned integer arithmetic
		addu			,	// add two registers								addu	(dest)	(a)		(b)		dest = a + b
		addui			,	// add register and immediate value					addui	(dest)	(a)		1.0		dest = a + 1
		subu			,	// subtract register from another					subu	(dest)	(a)		(b)		dest = a - b
		subui			,	// subtract immediate value from register			subui	(dest)	(a)		1.0		dest = a - 1
		subuir			,	// subtract register from immediate value			subuir	(dest)	(a)		1.0		dest = 1 - a
		mulu			,	// multiply two registers							mulu	(dest)	(a)		(b)		dest = a * b
		mului			,	// multiply register and immediate value			mului	(dest)	(a)		1.0		dest = a * 1
		divu			,	// divide register by another						divu	(dest)	(a)		(b)		dest = a / b
		divui			,	// divide register by immediate value				divui	(dest)	(a)		1.0		dest = a / 1
		divuir			,	// divide immediate value by register				divuir	(dest)	(a)		1.0		dest = 1 / a

		// integer / floating point conversion
		cvt_if			,	// convert integer to f32							cvt_if	(a)						a = f32(a)
		cvt_id			,	// convert integer to f64							cvt_id	(a)						a = f64(a)
		cvt_iu			,	// convert integer to uinteger						cvt_iu	(a)						a = uinteger_tp(a)
		cvt_uf			,	// convert uinteger to f32							cvt_if	(a)						a = f32(a)
		cvt_ud			,	// convert uinteger to f64							cvt_id	(a)						a = f64(a)
		cvt_ui			,	// convert uinteger to integer						cvt_ui	(a)						a = integer_tp(a)
		cvt_fi			,	// convert f32 to integer							cvt_fi 	(a)						a = integer_tp(a)
		cvt_fu			,	// convert f32 to uinteger							cvt_fi 	(a)						a = uinteger_tp(a)
		cvt_fd			,	// convert f32 to f64								cvt_fd 	(a)						a = f64(a)
		cvt_di			,	// convert f64 to integer							cvt_di 	(a)						a = integer_tp(a)
		cvt_du			,	// convert f64 to uinteger							cvt_di 	(a)						a = uinteger_tp(a)
		cvt_df			,	// convert f64 to f32								cvt_df 	(a)						a = f32(a)

		// floating point arithmetic (only accepts $fxx registers and floating point immediates)
		// f32
		fadd			,	// add two registers								fadd	(dest)	(a)		(b)		dest = a + b
		faddi			,	// add register and immediate value					faddi	(dest)	(a)		1.0		dest = a + 1.0
		fsub			,	// subtract register from another					fsub	(dest)	(a)		(b)		dest = a - b
		fsubi			,	// subtract immediate value from register			fsubi	(dest)	(a)		1.0		dest = a - 1.0
		fsubir			,	// subtract register from immediate value			fsubir	(dest)	(a)		1.0		dest = 1.0 - a
		fmul			,	// multiply two registers							fmul	(dest)	(a)		(b)		dest = a * b
		fmuli			,	// multiply register and immediate value			fmuli	(dest)	(a)		1.0		dest = a * 1.0
		fdiv			,	// divide register by another						fdiv	(dest)	(a)		(b)		dest = a / b
		fdivi			,	// divide register by immediate value				fdivi	(dest)	(a)		1.0		dest = a / 1.0
		fdivir			,	// divide immediate value by register				fdivir	(dest)	(a)		1.0		dest = 1.0 / a
		// f64
		dadd			,	// add two registers								fadd	(dest)	(a)		(b)		dest = a + b
		daddi			,	// add register and immediate value					faddi	(dest)	(a)		1.0		dest = a + 1.0
		dsub			,	// subtract register from another					fsub	(dest)	(a)		(b)		dest = a - b
		dsubi			,	// subtract immediate value from register			fsubi	(dest)	(a)		1.0		dest = a - 1.0
		dsubir			,	// subtract register from immediate value			fsubir	(dest)	(a)		1.0		dest = 1.0 - a
		dmul			,	// multiply two registers							fmul	(dest)	(a)		(b)		dest = a * b
		dmuli			,	// multiply register and immediate value			fmuli	(dest)	(a)		1.0		dest = a * 1.0
		ddiv			,	// divide register by another						fdiv	(dest)	(a)		(b)		dest = a / b
		ddivi			,	// divide register by immediate value				fdivi	(dest)	(a)		1.0		dest = a / 1.0
		ddivir			,	// divide immediate value by register				fdivir	(dest)	(a)		1.0		dest = 1.0 / a

		// comparison (need unsigned counterparts still)
		lt				,	// check if register less than register				lt		(dest)	(a)		(b)		dest = a < b
		lti				,	// check if register less than immediate			lti		(dest)	(a)		1		dest = a < 1
		lte				,	// check if register less than or equal register	lte		(dest)	(a)		(b)		dest = a <= b
		ltei			,	// check if register less than or equal immediate	ltei	(dest)	(a)		1		dest = a <= 1
		gt				,	// check if register greater than register			gt		(dest)	(a)		(b)		dest = a > b
		gti				,	// check if register greater than immediate			gti		(dest)	(a)		1		dest = a > 1
		gte				,	// check if register greater than or equal register	gte		(dest)	(a)		(b)		dest = a >= b
		gtei			,	// check if register greater than or equal imm.		gtei	(dest)	(a)		1		dest = a >= 1
		cmp				,	// check if register equal register					cmp		(dest)	(a)		(b)		dest = a == b
		cmpi			,	// check if register equal immediate				cmpi	(dest)	(a)		1		dest = a == 1
		ncmp			,	// check if register not equal register				ncmp	(dest)	(a)		(b)		dest = a != b
		ncmpi			,	// check if register not equal immediate			ncmpi	(dest)	(a)		1		dest = a != 1

		// floating point comparison
		// f32
		flt				,	// check if register less than register				flt		(dest)	(a)		(b)		dest = a < b
		flti			,	// check if register less than immediate			flti	(dest)	(a)		1.0		dest = a < 1.0
		flte			,	// check if register less than or equal register	flte	(dest)	(a)		(b)		dest = a <= b
		fltei			,	// check if register less than or equal immediate	fltei	(dest)	(a)		1.0		dest = a <= 1.0
		fgt				,	// check if register greater than register			fgt		(dest)	(a)		(b)		dest = a > b
		fgti			,	// check if register greater than immediate			fgti	(dest)	(a)		1.0		dest = a > 1.0
		fgte			,	// check if register greater than or equal register	fgte	(dest)	(a)		(b)		dest = a >= b
		fgtei			,	// check if register greater than or equal imm.		fgtei	(dest)	(a)		1.0		dest = a >= 1.0
		fcmp			,	// check if register equal register					fcmp	(dest)	(a)		(b)		dest = a == b
		fcmpi			,	// check if register equal immediate				fcmpi	(dest)	(a)		1.0		dest = a == 1.0
		fncmp			,	// check if register not equal register				fncmp	(dest)	(a)		(b)		dest = a != b
		fncmpi			,	// check if register not equal immediate			fncmpi	(dest)	(a)		1.0		dest = a != 1.0
		// f64
		dlt				,	// check if register less than register				flt		(dest)	(a)		(b)		dest = a < b
		dlti			,	// check if register less than immediate			flti	(dest)	(a)		1.0		dest = a < 1.0
		dlte			,	// check if register less than or equal register	flte	(dest)	(a)		(b)		dest = a <= b
		dltei			,	// check if register less than or equal immediate	fltei	(dest)	(a)		1.0		dest = a <= 1.0
		dgt				,	// check if register greater than register			fgt		(dest)	(a)		(b)		dest = a > b
		dgti			,	// check if register greater than immediate			fgti	(dest)	(a)		1.0		dest = a > 1.0
		dgte			,	// check if register greater than or equal register	fgte	(dest)	(a)		(b)		dest = a >= b
		dgtei			,	// check if register greater than or equal imm.		fgtei	(dest)	(a)		1.0		dest = a >= 1.0
		dcmp			,	// check if register equal register					fcmp	(dest)	(a)		(b)		dest = a == b
		dcmpi			,	// check if register equal immediate				fcmpi	(dest)	(a)		1.0		dest = a == 1.0
		dncmp			,	// check if register not equal register				fncmp	(dest)	(a)		(b)		dest = a != b
		dncmpi			,	// check if register not equal immediate			fncmpi	(dest)	(a)		1.0		dest = a != 1.0

		// boolean
		and				,	// logical and										and		(dest)	(a)		(b)		dest = a && b
		andi			,	// logical and										and		(dest)	(a)		1		dest = a && 1 (who knows)
		or				,	// logical or										or		(dest)	(a)		(b)		dest = a || b
		ori				,	// logical or										or		(dest)	(a)		1		dest = a || 1 (who knows)

		// bitwise
		band			,	// bitwise and										band	(dest)	(a)		(b)		dest = a & b
		bandi			,	// bitwise and register and immediate value			bandi	(dest)	(a)		0x0F	dest = a & 0x0F
		bor				,	// bitwise or										bor		(dest)	(a)		(b)		dest = a | b
		bori			,	// bitwise or register and immediate value			bori	(dest)	(a)		0x0F	dest = a | 0x0F
		xor				,	// exclusive or										xor		(dest)	(a)		(b)		dest = a ^ b
		xori			,	// exlusive or register and immediate value			xori	(dest)	(a)		0x0F	dest = a ^ 0x0F
		sl				,	// shift bits left by amount from register			sl		(dest)	(a)		(b)		dest = a << b
		sli				,	// shift bits left by immediate value				sli		(dest)	(a)		4		dest = a << 4
		slir			,	// shift bits of immediate left by register value	slir	(dest)	(a)		4		dest = 4 << a
		sr				,	// shift bits right by amount from register			sr		(dest)	(a)		(b)		dest = a >> b
		sri				,	// shift bits right by immediate value				sri		(dest)	(a)		4		dest = a >> 4
		srir			,	// shift bits of immediate right by register value	sri		(dest)	(a)		4		dest = 4 >> a

		// control flow
		beqz			,	// branch if register equals zero					beqz	(a)		(fail_addr)		if a: goto fail_addr
		bneqz			,	// branch if register not equals zero				bneqz	(a)		(fail_addr)		if !a: goto fail_addr
		bgtz			,	// branch if register greater than zero				bgtz	(a)		(fail_addr)		if a <= 0: goto fail_addr
		bgtez			,	// branch if register greater than or equals zero	bgtez	(a)		(fail_addr)		if a < 0: goto fail_addr
		bltz			,	// branch if register less than zero				bltz	(a)		(fail_addr)		if a >= 0: goto fail_addr
		bltez			,	// branch if register less than or equals zero		bltez	(a)		(fail_addr)		if a > 0: goto fail_addr
		jmp				,	// jump to address									jmp		0x123					$ip = 0x123
		jmpr			,	// jump to address in register						jmp		(a)						$ip = a
		jal				,	// jump to address and store $ip in $ra				jal		0x123					$ra = $ip + 1;$ip = 0x123
		jalr			,	// jump to address in register and store $ip in $ra	jalr	(a)						$ra = $ip + 1;$ip = a

		instruction_count	// number of instructions
	};

	extern const char* instruction_str[(i32)vm_instruction::instruction_count];
};