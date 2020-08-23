#include <vm.h>
#include <allocator.h>
#include <stdarg.h>
#include <string.h>
#include <context.h>

#include <asmjit/asmjit.h>
using namespace asmjit;

namespace gjs {
	char* fmt(vm_allocator* alloc, const char* fmt, ...) {
		u64 sz = strlen(fmt) + 1024;
		char* buf = (char*)alloc->allocate(sz);
		memset(buf, 0, sz);

		va_list a;
		va_start(a, fmt);
		vsnprintf(buf, sz, fmt, a);
		va_end(a);
		return buf;
	};


	using vmi = vm_instruction;
	using vmr = vm_register;
	
	#define GRx(r, x) *((x*)&state.registers[u8(r)])
	#define GRi(r) GRx(r, integer)
	#define GRd(r) GRx(r, decimal)
	#define GR8(r) GRx(r, u8)
	#define GR16(r) GRx(r, u16)
	#define GR32(r) GRx(r, u32)
	#define GR64(r) GRx(r, u64)
	#define _O1 decode_operand_1(i)
	#define _O1i decode_operand_1i(i)
	#define _O2 decode_operand_2(i)
	#define _O2i decode_operand_2i(i)
	#define _O3 decode_operand_3(i)
	#define _O3i decode_operand_3i(i)
	#define _O3f decode_operand_3f(i)

	vm::vm(vm_context* ctx, vm_allocator* allocator, u32 stack_size, u32 mem_size) : m_ctx(ctx), alloc(allocator), state(mem_size), m_stack_size(stack_size) {
	}

	vm::~vm() {
	}

	void vm::execute(const instruction_array& code, address entry) {
		jit(code);
		
		GRi(vmr::ip) = (integer)entry;
		GRi(vmr::ra) = 0;
		GRi(vmr::sp) = 0;
		GRi(vmr::zero) = 0;

		instruction i;
		integer* ip = &GRi(vmr::ip);
		u32 cs = code.size();
		bool term = false;
		while ((*ip) <= cs && !term) {
			i = code[*ip];
			printf("%2.2d: %s\n", *ip, instruction_to_string(i, &state).c_str());

			vmi instr = decode_instruction(i);
			switch (instr) {
				// do nothing
                case vmi::null: {
                	break;
                }
				// this is only here to keep the switch cases tight
				case vmi::term: {
					term = true;
					break;
				}
                // load 1 byte from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld8: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					GR8(_O1) = *(u8*)state.memory[offset];
                    break;
                }
                // load 2 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld16: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 2) {
						// exception
						break;
					}
					GR16(_O1) = *(u16*)state.memory[offset];
                    break;
                }
                // load 4 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld32: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 4) {
						// exception
						break;
					}
					GR32(_O1) = *(u32*)state.memory[offset];
                    break;
                }
                // load 8 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld64: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 8) {
						// exception
						break;
					}
					GR64(_O1) = *(u64*)state.memory[offset];
                    break;
                }
                // store 1 byte in memory from register				store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st8: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u8*)state.memory[offset]) = GR8(_O1);
                    break;
                }
                // store 2 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st16: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u16*)state.memory[offset]) = GR16(_O1);
                    break;
                }
                // store 4 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st32: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u32*)state.memory[offset]) = GR32(_O1);
                    break;
                }
                // store 8 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st64: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u64*)state.memory[offset]) = GR64(_O1);
                    break;
                }
                // push register onto stack							push	(a)						*sp = a; sp--
                case vmi::push: {
					integer& offset = GRi(vmr::sp);
					if (offset - 8 <= 0) {
						// stack overflow exception
						break;
					}

					*((u64*)state.memory[offset]) = GR64(_O1);
					offset -= 8;
                    break;
                }
                // pop value from the stack into register			pop		(a)						a = *sp; sp++
                case vmi::pop: {
					integer& offset = GRi(vmr::sp);
					if (offset + 8 > m_stack_size) {
						// stack underflow exception
						break;
					}
					GR64(_O1) = *((u64*)state.memory[offset]);
					offset += 8;
                    break;
                }
				// move from general register to float register		mtfp	(a)		(b)				b = a
				case vmi::mtfp: {
					GR32(_O2) = GR32(_O1);
					break;
				}
				// move from float register to general register		mffp	(a)		(b)				b =	a
				case vmi::mffp: {
					GR32(_O2) = GR32(_O1);
					break;
				}
                // add two registers								add		(dest)	(a)		(b)		dest = a + b
                case vmi::add: {
					GRi(_O1) = GRi(_O2) + GRi(_O3);
                    break;
                }
                // add register and immediate value					addi	(dest)	(a)		1.0		dest = a + 1
                case vmi::addi: {
					GRi(_O1) = GRi(_O2) + _O3i;
					break;
                }
                // subtract register from another					sub		(dest)	(a)		(b)		dest = a - b
                case vmi::sub: {
					GRi(_O1) = GRi(_O2) - GRi(_O3);
                    break;
                }
                // subtract immediate value from register			subi	(dest)	(a)		1.0		dest = a - 1
                case vmi::subi: {
					GRi(_O1) = GRi(_O2) - _O3i;
                    break;
                }
                // subtract register from immediate value			subir	(dest)	(a)		1.0		dest = 1 - a
                case vmi::subir: {
					GRi(_O1) = _O3i - GRi(_O2);
                    break;
                }
                // multiply two registers							mul		(dest)	(a)		(b)		dest = a * b
                case vmi::mul: {
					GRi(_O1) = GRi(_O2) * GRi(_O3);
                    break;
                }
                // multiply register and immediate value			muli	(dest)	(a)		1.0		dest = a * 1
                case vmi::muli: {
					GRi(_O1) = GRi(_O2) * _O3i;
                    break;
                }
                // divide register by another						div		(dest)	(a)		(b)		dest = a / b
                case vmi::div: {
					GRi(_O1) = GRi(_O2) / GRi(_O3);
                    break;
                }
                // divide register by immediate value				divi	(dest)	(a)		1.0		dest = a / 1
                case vmi::divi: {
					GRi(_O1) = GRi(_O2) / _O3i;
                    break;
                }
                // divide immediate value by register				divir	(dest)	(a)		1.0		dest = 1 / a
                case vmi::divir: {
					GRi(_O1) = _O3i / GRi(_O2);
                    break;
                }
                // add two registers								add		(dest)	(a)		(b)		dest = a + b
				case vmi::fadd: {
					GRd(_O1) = GRd(_O2) + GRd(_O3);
                    break;
                }
                // add register and immediate value					faddi	(dest)	(a)		1.0		dest = a + 1.0
                case vmi::faddi: {
					GRd(_O1) = GRd(_O2) + _O3f;
					break;
                }
                // subtract register from another					fsub	(dest)	(a)		(b)		dest = a - b
                case vmi::fsub: {
					GRd(_O1) = GRd(_O2) - GRd(_O3);
                    break;
                }
                // subtract immediate value from register			fsubi	(dest)	(a)		1.0		dest = a - 1.0
                case vmi::fsubi: {
					GRd(_O1) = GRd(_O2) - _O3f;
                    break;
                }
                // subtract register from immediate value			fsubir	(dest)	(a)		1.0		dest = 1.0 - a
                case vmi::fsubir: {
					GRd(_O1) = _O3f - GRd(_O2);
                    break;
                }
                // multiply two registers							fmul	(dest)	(a)		(b)		dest = a * b
                case vmi::fmul: {
					GRd(_O1) = GRd(_O2) * GRd(_O3);
                    break;
                }
                // multiply register and immediate value			fmuli	(dest)	(a)		1.0		dest = a * 1.0
                case vmi::fmuli: {
					GRd(_O1) = GRd(_O2) * _O3f;
                    break;
                }
                // divide register by another						fdiv	(dest)	(a)		(b)		dest = a / b
                case vmi::fdiv: {
					GRd(_O1) = GRd(_O2) / GRd(_O3);
                    break;
                }
                // divide register by immediate value				fdivi	(dest)	(a)		1.0		dest = a / 1.0
                case vmi::fdivi: {
					GRd(_O1) = GRd(_O2) / _O3f;
                    break;
                }
                // divide immediate value by register				fdivir	(dest)	(a)		1.0		dest = 1.0 / a
                case vmi::fdivir: {
					GRd(_O1) = _O3f / GRd(_O2);
                    break;
                }
                // logical and										and		(dest)	(a)		(b)		dest = a && b
                case vmi::and: {
					GR64(_O1) = GR64(_O2) && GR64(_O3);
                    break;
                }
                // logical or										or		(dest)	(a)		(b)		dest = a || b
                case vmi::or: {
					GR64(_O1) = GR64(_O2) || GR64(_O3);
                    break;
                }
                // bitwise and										band	(dest)	(a)		(b)		dest = a & b
                case vmi::band: {
					GR64(_O1) = GR64(_O2) & GR64(_O3);
                    break;
                }
                // bitwise and register and immediate value			bandi	(dest)	(a)		0x0F	dest = a & 0x0F
                case vmi::bandi: {
					GR64(_O1) = GR64(_O2) & _O3i;
                    break;
                }
                // bitwise or										bor		(dest)	(a)		(b)		dest = a | b
                case vmi::bor: {
					GR64(_O1) = GR64(_O2) | GR64(_O3);
                    break;
                }
                // bitwise or register and immediate value			bori	(dest)	(a)		0x0F	dest = a | 0x0F
                case vmi::bori: {
					GR64(_O1) = GR64(_O2) | _O3i;
                    break;
                }
                // exclusive or										xor		(dest)	(a)		(b)		dest = a ^ b
                case vmi::xor: {
					GR64(_O1) = GR64(_O2) ^ GR64(_O3);
                    break;
                }
                // exlusive or register and immediate value			xori	(dest)	(a)		0x0F	dest = a ^ 0x0F
                case vmi::xori: {
					GR64(_O1) = GR64(_O2) ^ _O3i;
                    break;
                }
                // shift bits left by amount from register			sl		(dest)	(a)		(b)		dest = a << b
                case vmi::sl: {
					GR64(_O1) = GR64(_O2) << GR64(_O3);
                    break;
                }
                // shift bits left by immediate value				sli		(dest)	(a)		4		dest = a << 4
                case vmi::sli: {
					GR64(_O1) = GR64(_O2) << _O3i;
                    break;
                }
                // shift bits right by amount from register			sr		(dest)	(a)		(b)		dest = a >> b
                case vmi::sr: {
					GR64(_O1) = GR64(_O2) >> GR64(_O3);
                    break;
                }
                // shift bits right by immediate value				sri		(dest)	(a)		4		dest = a >> 4
                case vmi::sri: {
					GR64(_O1) = GR64(_O2) >> _O3i;
                    break;
                }
                // branch if register equals zero					beqz	(a)		(fail_addr)		if a: goto fail_addr
                case vmi::beqz: {
					if(GRi(_O1)) *ip = _O2i - 1;
                    break;
                }
                // branch if register not equals zero				bneqz	(a)		(fail_addr)		if !a: goto fail_addr
                case vmi::bneqz: {
					if(!GRi(_O1)) *ip = _O2i - 1;
                    break;
                }
                // branch if register greater than zero				bgtz	(a)		(fail_addr)		if a <= 0: goto fail_addr
                case vmi::bgtz: {
					if(GRi(_O1) <= 0) *ip = _O2i - 1;
                    break;
                }
                // branch if register greater than or equals zero	bgtez	(a)		(fail_addr)		if a < 0: goto fail_addr
                case vmi::bgtez: {
					if(GRi(_O1) < 0) *ip = _O2i - 1;
                    break;
                }
                // branch if register less than zero				bltz	(a)		(fail_addr)		if a >= 0: goto fail_addr
                case vmi::bltz: {
					if(GRi(_O1) >= 0) *ip = _O2i - 1;
                    break;
                }
                // branch if register less than or equals zero		bltez	(a)		(fail_addr)		if a > 0: goto fail_addr
                case vmi::bltez: {
					if(GRi(_O1) > 0) *ip = _O2i - 1;
                    break;
                }
                // jump to address									jmp		0x123					$ip = 0x123
                case vmi::jmp: {
					*ip = _O1i - 1;
                    break;
                }
                // jump to address in register						jmp		(a)						$ip = a
                case vmi::jmpr: {
					*ip = GRi(_O1) - 1;
                    break;
                }
                // jump to address and store $ip in $ra				jal		0x123					$ra = $ip + 1;$ip = 0x123
                case vmi::jal: {
					GRi(vmr::ra) = (*ip) + 1;
					*ip = _O1i - 1;
                    break;
                }
                // jump to address in register and store $ip in $ra	jalr	(a)						$ra = $ip + 1;$ip = a
                case vmi::jalr: {
					GRi(vmr::ra) = (*ip) + 1;
					*ip = GRi(_O1) - 1;
                    break;
                }
				case vmi::instruction_count: {
                    throw runtime_exception(m_ctx, "Invalid Instruction");
					break;
				}
				default: {
					throw runtime_exception(m_ctx, "Invalid Instruction");
					// deinitialize?
					break;
				}
			}

            (*ip)++;
		}
	}

	void vm::jit(const instruction_array& code) {
		/*

		CodeHolder h;
		x86::Compiler c(&h);
		c.newFunc(FuncSignatureT<void>(CallConv::kIdCDecl));

		for (u32 ip = 0;ip < code.size();ip++) {
			instruction i = code[ip];
			vmi instr = decode_instruction(i);
			switch (instr) {
				// do nothing
                case vmi::null: {
                	break;
                }
				// this is only here to keep the switch cases tight
				case vmi::term: {
					break;
				}
                // load 1 byte from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld8: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					GR8(_O1) = *(u8*)state.memory[offset];
                    break;
                }
                // load 2 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld16: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 2) {
						// exception
						break;
					}
					GR16(_O1) = *(u16*)state.memory[offset];
                    break;
                }
                // load 4 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld32: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 4) {
						// exception
						break;
					}
					GR32(_O1) = *(u32*)state.memory[offset];
                    break;
                }
                // load 8 bytes from memory into register			ld8		(dest)	(src)	0		dest = *(src + 0)
                case vmi::ld64: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size() - 8) {
						// exception
						break;
					}
					GR64(_O1) = *(u64*)state.memory[offset];
                    break;
                }
                // store 1 byte in memory from register				store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st8: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u8*)state.memory[offset]) = GR8(_O1);
                    break;
                }
                // store 2 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st16: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u16*)state.memory[offset]) = GR16(_O1);
                    break;
                }
                // store 4 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st32: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u32*)state.memory[offset]) = GR32(_O1);
                    break;
                }
                // store 8 bytes in memory from register			store	(src)	(dest)	10		dest = *(src + 10)
                case vmi::st64: {
					integer offset = GRi(integer(_O2)) + _O3i;
					if (offset < 0 || offset >= state.memory.size()) {
						// exception
						break;
					}
					*((u64*)state.memory[offset]) = GR64(_O1);
                    break;
                }
                // push register onto stack							push	(a)						*sp = a; sp--
                case vmi::push: {
					integer& offset = GRi(vmr::sp);
					if (offset - 8 <= 0) {
						// stack overflow exception
						break;
					}

					*((u64*)state.memory[offset]) = GR64(_O1);
					offset -= 8;
                    break;
                }
                // pop value from the stack into register			pop		(a)						a = *sp; sp++
                case vmi::pop: {
					integer& offset = GRi(vmr::sp);
					if (offset + 8 > m_stack_size) {
						// stack underflow exception
						break;
					}
					GR64(_O1) = *((u64*)state.memory[offset]);
					offset += 8;
                    break;
                }
				// move from general register to float register		mtfp	(a)		(b)				b = a
				case vmi::mtfp: {
					GR32(_O2) = GR32(_O1);
					break;
				}
				// move from float register to general register		mffp	(a)		(b)				b =	a
				case vmi::mffp: {
					GR32(_O2) = GR32(_O1);
					break;
				}
                // add two registers								add		(dest)	(a)		(b)		dest = a + b
                case vmi::add: {
					GRi(_O1) = GRi(_O2) + GRi(_O3);
                    break;
                }
                // add register and immediate value					addi	(dest)	(a)		1.0		dest = a + 1
                case vmi::addi: {
					GRi(_O1) = GRi(_O2) + _O3i;
					break;
                }
                // subtract register from another					sub		(dest)	(a)		(b)		dest = a - b
                case vmi::sub: {
					GRi(_O1) = GRi(_O2) - GRi(_O3);
                    break;
                }
                // subtract immediate value from register			subi	(dest)	(a)		1.0		dest = a - 1
                case vmi::subi: {
					GRi(_O1) = GRi(_O2) - _O3i;
                    break;
                }
                // subtract register from immediate value			subir	(dest)	(a)		1.0		dest = 1 - a
                case vmi::subir: {
					GRi(_O1) = _O3i - GRi(_O2);
                    break;
                }
                // multiply two registers							mul		(dest)	(a)		(b)		dest = a * b
                case vmi::mul: {
					GRi(_O1) = GRi(_O2) * GRi(_O3);
                    break;
                }
                // multiply register and immediate value			muli	(dest)	(a)		1.0		dest = a * 1
                case vmi::muli: {
					GRi(_O1) = GRi(_O2) - _O3i;
                    break;
                }
                // divide register by another						div		(dest)	(a)		(b)		dest = a / b
                case vmi::div: {
					GRi(_O1) = GRi(_O2) / GRi(_O3);
                    break;
                }
                // divide register by immediate value				divi	(dest)	(a)		1.0		dest = a / 1
                case vmi::divi: {
					GRi(_O1) = GRi(_O2) / _O3i;
                    break;
                }
                // divide immediate value by register				divir	(dest)	(a)		1.0		dest = 1 / a
                case vmi::divir: {
					GRi(_O1) = _O3i / GRi(_O2);
                    break;
                }
                // add two registers								add		(dest)	(a)		(b)		dest = a + b
				case vmi::fadd: {
					GRd(_O1) = GRd(_O2) + GRd(_O3);
                    break;
                }
                // add register and immediate value					faddi	(dest)	(a)		1.0		dest = a + 1.0
                case vmi::faddi: {
					GRd(_O1) = GRd(_O2) + _O3f;
					break;
                }
                // subtract register from another					fsub	(dest)	(a)		(b)		dest = a - b
                case vmi::fsub: {
					GRd(_O1) = GRd(_O2) - GRd(_O3);
                    break;
                }
                // subtract immediate value from register			fsubi	(dest)	(a)		1.0		dest = a - 1.0
                case vmi::fsubi: {
					GRd(_O1) = GRd(_O2) - _O3f;
                    break;
                }
                // subtract register from immediate value			fsubir	(dest)	(a)		1.0		dest = 1.0 - a
                case vmi::fsubir: {
					GRd(_O1) = _O3f - GRd(_O2);
                    break;
                }
                // multiply two registers							fmul	(dest)	(a)		(b)		dest = a * b
                case vmi::fmul: {
					GRd(_O1) = GRd(_O2) * GRd(_O3);
                    break;
                }
                // multiply register and immediate value			fmuli	(dest)	(a)		1.0		dest = a * 1.0
                case vmi::fmuli: {
					GRd(_O1) = GRd(_O2) * _O3f;
                    break;
                }
                // divide register by another						fdiv	(dest)	(a)		(b)		dest = a / b
                case vmi::fdiv: {
					GRd(_O1) = GRd(_O2) / GRd(_O3);
                    break;
                }
                // divide register by immediate value				fdivi	(dest)	(a)		1.0		dest = a / 1.0
                case vmi::fdivi: {
					GRd(_O1) = GRd(_O2) / _O3f;
                    break;
                }
                // divide immediate value by register				fdivir	(dest)	(a)		1.0		dest = 1.0 / a
                case vmi::fdivir: {
					GRd(_O1) = _O3f / GRd(_O2);
                    break;
                }
                // logical and										and		(dest)	(a)		(b)		dest = a && b
                case vmi::and: {
					GR64(_O1) = GR64(_O2) && GR64(_O3);
                    break;
                }
                // logical or										or		(dest)	(a)		(b)		dest = a || b
                case vmi::or: {
					GR64(_O1) = GR64(_O2) || GR64(_O3);
                    break;
                }
                // bitwise and										band	(dest)	(a)		(b)		dest = a & b
                case vmi::band: {
					GR64(_O1) = GR64(_O2) & GR64(_O3);
                    break;
                }
                // bitwise and register and immediate value			bandi	(dest)	(a)		0x0F	dest = a & 0x0F
                case vmi::bandi: {
					GR64(_O1) = GR64(_O2) & _O3i;
                    break;
                }
                // bitwise or										bor		(dest)	(a)		(b)		dest = a | b
                case vmi::bor: {
					GR64(_O1) = GR64(_O2) | GR64(_O3);
                    break;
                }
                // bitwise or register and immediate value			bori	(dest)	(a)		0x0F	dest = a | 0x0F
                case vmi::bori: {
					GR64(_O1) = GR64(_O2) | _O3i;
                    break;
                }
                // exclusive or										xor		(dest)	(a)		(b)		dest = a ^ b
                case vmi::xor: {
					GR64(_O1) = GR64(_O2) ^ GR64(_O3);
                    break;
                }
                // exlusive or register and immediate value			xori	(dest)	(a)		0x0F	dest = a ^ 0x0F
                case vmi::xori: {
					GR64(_O1) = GR64(_O2) ^ _O3i;
                    break;
                }
                // shift bits left by amount from register			sl		(dest)	(a)		(b)		dest = a << b
                case vmi::sl: {
					GR64(_O1) = GR64(_O2) << GR64(_O3);
                    break;
                }
                // shift bits left by immediate value				sli		(dest)	(a)		4		dest = a << 4
                case vmi::sli: {
					GR64(_O1) = GR64(_O2) << _O3i;
                    break;
                }
                // shift bits right by amount from register			sr		(dest)	(a)		(b)		dest = a >> b
                case vmi::sr: {
					GR64(_O1) = GR64(_O2) >> GR64(_O3);
                    break;
                }
                // shift bits right by immediate value				sri		(dest)	(a)		4		dest = a >> 4
                case vmi::sri: {
					GR64(_O1) = GR64(_O2) >> _O3i;
                    break;
                }
                // branch if register equals zero					beqz	(a)		(fail_addr)		if a: goto fail_addr
                case vmi::beqz: {
					if(GRi(_O1)) *ip = _O2i;
                    break;
                }
                // branch if register not equals zero				bneqz	(a)		(fail_addr)		if !a: goto fail_addr
                case vmi::bneqz: {
					if(!GRi(_O1)) *ip = _O2i;
                    break;
                }
                // branch if register greater than zero				bgtz	(a)		(fail_addr)		if a <= 0: goto fail_addr
                case vmi::bgtz: {
					if(GRi(_O1) <= 0) *ip = _O2i;
                    break;
                }
                // branch if register greater than or equals zero	bgtez	(a)		(fail_addr)		if a < 0: goto fail_addr
                case vmi::bgtez: {
					if(GRi(_O1) < 0) *ip = _O2i;
                    break;
                }
                // branch if register less than zero				bltz	(a)		(fail_addr)		if a >= 0: goto fail_addr
                case vmi::bltz: {
					if(GRi(_O1) >= 0) *ip = _O2i;
                    break;
                }
                // branch if register less than or equals zero		bltez	(a)		(fail_addr)		if a > 0: goto fail_addr
                case vmi::bltez: {
					if(GRi(_O1) > 0) *ip = _O2i;
                    break;
                }
                // jump to address									jmp		0x123					$ip = 0x123
                case vmi::jmp: {
					*ip = _O1i;
                    break;
                }
                // jump to address in register						jmp		(a)						$ip = a
                case vmi::jmpr: {
					*ip = GRi(_O1);
                    break;
                }
                // jump to address and store $ip in $ra				jal		0x123					$ra = $ip + 1;$ip = 0x123
                case vmi::jal: {
					GRi(vmr::ra) = (*ip) + 1;
					*ip = _O1i;
                    break;
                }
                // jump to address in register and store $ip in $ra	jalr	(a)						$ra = $ip + 1;$ip = a
                case vmi::jalr: {
					GRi(vmr::ra) = (*ip) + 1;
					*ip = GRi(_O1);
                    break;
                }
				case vmi::instruction_count: {
					break;
				}
				default: {
					// exception
					// deinitialize?
					break;
				}
			}
		}

		*/
	}
};