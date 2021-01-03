#include <gjs/vm/vm.h>
#include <gjs/vm/allocator.h>
#include <stdarg.h>
#include <string.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/backends/b_vm.h>

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
    #define GRi(r) GRx(r, i64)
    #define GRd(r) GRx(r, f64)
    #define GRf(r) GRx(r, f32)
    #define GR8(r) GRx(r, u8)
    #define GR16(r) GRx(r, u16)
    #define GR32(r) GRx(r, u32)
    #define GR64(r) GRx(r, u64)
    #define _O1 i.op_1r()
    #define _O1i i.imm_i()
    #define _O1ui i.imm_u()
    #define _O1ui64 i.imm_u()
    #define _O2 i.op_2r()
    #define _O2i i.imm_i()
    #define _O2ui i.imm_u()
    #define _O3 i.op_3r()
    #define _O3i i.imm_i()
    #define _O3ui i.imm_u()
    #define _O3ui64 i.imm_u()
    #define _O3f f32(i.imm_f())
    #define _O3d i.imm_f()

    #define STACK_PADDING_SIZE 8

    vm::vm(vm_backend* ctx, vm_allocator* allocator, u32 stack_size, u32 mem_size) : m_ctx(ctx), alloc(allocator), state(mem_size), m_stack_size(stack_size) {
    }

    vm::~vm() {
    }

    void vm::execute(const instruction_array& code, address entry) {
        GR64(vmr::ip) = entry;
        GR64(vmr::ra) = 0;
        GR64(vmr::sp) = (u64)state.memory[0];
        GR64(vmr::zero) = 0;

        u64 stack_padding_start = ((u64)state.memory[0]) + m_stack_size;
        u64 stack_padding_end = stack_padding_start + STACK_PADDING_SIZE;

        instruction i;
        u64* ip = &GRx(vmr::ip, u64);
        u64 cs = code.size();
        bool term = false;
        while ((*ip) <= cs && !term) {
            i = code[*ip];
            if (m_ctx->log_instructions()) {
                printf("0x%2.2llx: %s\n", *ip, i.to_string(m_ctx).c_str());
            }

            vmi instr = i.instr();
            switch (instr) {
                // do nothing
                case vmi::null: {
                    break;
                }
                // terminate execution
                case vmi::term: {
                    term = true;
                    break;
                }
                // load 1 byte from memory into register            ld8        (dest)    (src)    0        dest = *(src + 0)
                case vmi::ld8: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u8* ptr = (u8*)offset;
                    GR64(_O1) = *(u8*)ptr;
                    break;
                }
                // load 2 bytes from memory into register           ld8        (dest)    (src)    0        dest = *(src + 0)
                case vmi::ld16: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u16* ptr = (u16*)offset;
                    GR64(_O1) = *(u16*)ptr;
                    break;
                }
                // load 4 bytes from memory into register           ld8        (dest)    (src)    0        dest = *(src + 0)
                case vmi::ld32: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u32* ptr = (u32*)offset;
                    GR64(_O1) = *(u32*)ptr;
                    break;
                }
                // load 8 bytes from memory into register           ld8        (dest)    (src)    0        dest = *(src + 0)
                case vmi::ld64: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u64* ptr = (u64*)offset;
                    GR64(_O1) = *(u64*)ptr;
                    break;
                }
                // store 1 byte in memory from register             store    (src)    (dest)    10        dest = *(src + 10)
                case vmi::st8: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u8* ptr = (u8*)offset;
                    *ptr = GR8(_O1);
                    break;
                }
                // store 2 bytes in memory from register            store    (src)    (dest)    10        dest = *(src + 10)
                case vmi::st16: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u16* ptr = (u16*)offset;
                    *ptr = GR16(_O1);
                    break;
                }
                // store 4 bytes in memory from register            store    (src)    (dest)    10        dest = *(src + 10)
                case vmi::st32: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u32* ptr = (u32*)offset;
                    *ptr = GR32(_O1);
                    break;
                }
                // store 8 bytes in memory from register            store    (src)    (dest)    10        dest = *(src + 10)
                case vmi::st64: {
                    u64 offset = GR64(integer(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw vm_exception(m_ctx, "Stack overflow detected");
                    }
                    u64* ptr = (u64*)offset;
                    *ptr = GR64(_O1);
                    break;
                }
                // store address of module data in register         mptr    (dest)               0xf    dest = modules[$v3] + 0xf    ($v3 must be module id)
                case vmi::mptr: {
                    script_module* mod = m_ctx->context()->module((u32)GRx(vmr::v3, u64));
                    if (!mod) throw vm_exception(m_ctx, "Invalid module id");
                    GR64(_O1) = (u64)mod->data()->data(_O2ui);
                    break;
                }
                // move from general register to float register     mtfp    (a)        (b)                b = a
                case vmi::mtfp: {
                    GR64(_O2) = GR64(_O1);
                    break;
                }
                // move from float register to general register     mffp    (a)        (b)                b =    a
                case vmi::mffp: {
                    GR64(_O2) = GR64(_O1);
                    break;
                }
                // add two registers                                add        (dest)    (a)        (b)        dest = a + b
                case vmi::add: {
                    GRx(_O1, i64) = GRx(_O2, i64) + GRx(_O3, i64);
                    break;
                }
                // add register and immediate value                 addi    (dest)    (a)        1.0        dest = a + 1
                case vmi::addi: {
                    GRx(_O1, i64) = GRx(_O2, i64) + _O3i;
                    break;
                }
                // subtract register from another                   sub        (dest)    (a)        (b)        dest = a - b
                case vmi::sub: {
                    GRx(_O1, i64) = GRx(_O2, i64) - GRx(_O3, i64);
                    break;
                }
                // subtract immediate value from register           subi    (dest)    (a)        1.0        dest = a - 1
                case vmi::subi: {
                    GRx(_O1, i64) = GRx(_O2, i64) - _O3i;
                    break;
                }
                // subtract register from immediate value           subir    (dest)    (a)        1.0        dest = 1 - a
                case vmi::subir: {
                    GRx(_O1, i64) = _O3i - GRx(_O2, i64);
                    break;
                }
                // multiply two registers                           mul        (dest)    (a)        (b)        dest = a * b
                case vmi::mul: {
                    GRx(_O1, i64) = GRx(_O2, i64) * GRi(_O3);
                    break;
                }
                // multiply register and immediate value            muli    (dest)    (a)        1.0        dest = a * 1
                case vmi::muli: {
                    GRx(_O1, i64) = GRx(_O2, i64) * _O3i;
                    break;
                }
                // divide register by another                       div        (dest)    (a)        (b)        dest = a / b
                case vmi::div: {
                    GRx(_O1, i64) = GRx(_O2, i64) / GRx(_O3, i64);
                    break;
                }
                // divide register by immediate value               divi    (dest)    (a)        1.0        dest = a / 1
                case vmi::divi: {
                    GRx(_O1, i64) = GRx(_O2, i64) / _O3i;
                    break;
                }
                // divide immediate value by register               divir    (dest)    (a)        1.0        dest = 1 / a
                case vmi::divir: {
                    GRx(_O1, i64) = _O3i / GRx(_O2, i64);
                    break;
                }
                // add two registers                                add        (dest)    (a)        (b)        dest = a + b
                case vmi::addu: {
                    GRx(_O1, u64) = GRx(_O2, u64) + GRx(_O3, u64);
                    break;
                }
                // add register and immediate value                 addi    (dest)    (a)        1.0        dest = a + 1
                case vmi::addui: {
                    u64* raddr = &GRx(_O1, u64);
                    GRx(_O1, u64) = GRx(_O2, u64) + _O3ui;
                    break;
                }
                // subtract register from another                   sub        (dest)    (a)        (b)        dest = a - b
                case vmi::subu: {
                    GRx(_O1, u64) = GRx(_O2, u64) - GRx(_O3, u64);
                    break;
                }
                // subtract immediate value from register           subi    (dest)    (a)        1.0        dest = a - 1
                case vmi::subui: {
                    GRx(_O1, u64) = GRx(_O2, u64) - _O3ui;
                    break;
                }
                // subtract register from immediate value           subir    (dest)    (a)        1.0        dest = 1 - a
                case vmi::subuir: {
                    GRx(_O1, u64) = _O3ui - GRx(_O2, u64);
                    break;
                }
                // multiply two registers                           mul        (dest)    (a)        (b)        dest = a * b
                case vmi::mulu: {
                    GRx(_O1, u64) = GRx(_O2, u64) * GRx(_O3, u64);
                    break;
                }
                // multiply register and immediate value            muli    (dest)    (a)        1.0        dest = a * 1
                case vmi::mului: {
                    GRx(_O1, u64) = GRx(_O2, u64) * _O3ui;
                    break;
                }
                // divide register by another                       div        (dest)    (a)        (b)        dest = a / b
                case vmi::divu: {
                    GRx(_O1, u64) = GRx(_O2, u64) / GRx(_O3, u64);
                    break;
                }
                // divide register by immediate value               divi    (dest)    (a)        1.0        dest = a / 1
                case vmi::divui: {
                    GRx(_O1, u64) = GRx(_O2, u64) / _O3ui;
                    break;
                }
                // divide immediate value by register               divir    (dest)    (a)        1.0        dest = 1 / a
                case vmi::divuir: {
                    GRx(_O1, u64) = _O3ui / GRx(_O2, u64);
                    break;
                }
                // convert integer to f32                           cvt_if    (a)                        a = f32(a)
                case vmi::cvt_if: {
                    i64* r = &GRx(_O1, i64);
                    f32 v = (f32)*r;
                    *r = 0;
                    (*(f32*)r) = v;
                    break;
                }
                // convert integer to f64                           cvt_id    (a)                        a = f64(a)
                case vmi::cvt_id: {
                    i64* r = &GRx(_O1, i64);
                    (*(f64*)r) = (f64)*r;
                    break;
                }
                // convert integer to uinteger                      cvt_iu    (a)                        a = uinteger_tp(a)
                case vmi::cvt_iu: {
                    i64* r = &GRx(_O1, i64);
                    (*(u64*)r) = *r;
                    break;
                }
                // convert uinteger to f32                          cvt_if    (a)                        a = f32(a)
                case vmi::cvt_uf: {
                    u64* r = &GRx(_O1, u64);
                    f32 v = (f32)*r;
                    *r = 0;
                    (*(f32*)r) = v;
                    break;
                }
                // convert uinteger to f64                          cvt_id    (a)                        a = f64(a)
                case vmi::cvt_ud: {
                    u64* r = &GRx(_O1, u64);
                    (*(f64*)r) = (f64)*r;
                    break;
                }
                // convert uinteger to integer                      cvt_iu    (a)                        a = integer_tp(a)
                case vmi::cvt_ui: {
                    u64* r = &GRx(_O1, u64);
                    (*(i64*)r) = *r;
                    break;
                }
                // convert f32 to integer                           cvt_fi     (a)                        a = integer_tp(a)
                case vmi::cvt_fi: {
                    f32* r = &GRx(_O1, f32);
                    (*(i64*)r) = (i64)*r;
                    break;
                }
                // convert f32 to uinteger                          cvt_fi     (a)                        a = uinteger_tp(a)
                case vmi::cvt_fu: {
                    f32* r = &GRx(_O1, f32);
                    (*(u64*)r) = (u64)*r;
                    break;
                }
                // convert f32 to f64                               cvt_fd     (a)                        a = f64(a)
                case vmi::cvt_fd: {
                    f32* r = &GRx(_O1, f32);
                    (*(f64*)r) = *r;
                    break;
                }
                // convert f64 to integer                           cvt_di     (a)                        a = integer_tp(a)
                case vmi::cvt_di: {
                    f64* r = &GRx(_O1, f64);
                    (*(i64*)r) = (i64)*r;
                    break;
                }
                // convert f64 to uinteger                          cvt_di     (a)                        a = uinteger_tp(a)
                case vmi::cvt_du: {
                    f64* r = &GRx(_O1, f64);
                    (*(u64*)r) = (u64)*r;
                    break;
                }
                // convert f64 to f32                               cvt_df     (a)                        a = f32(a)
                case vmi::cvt_df: {
                    f64* r = &GRx(_O1, f64);
                    f32 v = (f32)*r;
                    (*(u64*)r) = 0;
                    (*(f32*)r) = v;
                    break;
                }
                // add two registers                                add        (dest)    (a)        (b)        dest = a + b
                case vmi::fadd: {
                    GRf(_O1) = GRf(_O2) + GRf(_O3);
                    break;
                }
                // add register and immediate value                 faddi    (dest)    (a)        1.0        dest = a + 1.0
                case vmi::faddi: {
                    GRf(_O1) = GRf(_O2) + _O3f;
                    break;
                }
                // subtract register from another                   fsub    (dest)    (a)        (b)        dest = a - b
                case vmi::fsub: {
                    GRf(_O1) = GRf(_O2) - GRf(_O3);
                    break;
                }
                // subtract immediate value from register           fsubi    (dest)    (a)        1.0        dest = a - 1.0
                case vmi::fsubi: {
                    GRf(_O1) = GRf(_O2) - _O3f;
                    break;
                }
                // subtract register from immediate value           fsubir    (dest)    (a)        1.0        dest = 1.0 - a
                case vmi::fsubir: {
                    GRf(_O1) = _O3f - GRf(_O2);
                    break;
                }
                // multiply two registers                           fmul    (dest)    (a)        (b)        dest = a * b
                case vmi::fmul: {
                    GRf(_O1) = GRf(_O2) * GRf(_O3);
                    break;
                }
                // multiply register and immediate value            fmuli    (dest)    (a)        1.0        dest = a * 1.0
                case vmi::fmuli: {
                    GRf(_O1) = GRf(_O2) * _O3f;
                    break;
                }
                // divide register by another                       fdiv    (dest)    (a)        (b)        dest = a / b
                case vmi::fdiv: {
                    GRf(_O1) = GRf(_O2) / GRf(_O3);
                    break;
                }
                // divide register by immediate value               fdivi    (dest)    (a)        1.0        dest = a / 1.0
                case vmi::fdivi: {
                    GRf(_O1) = GRf(_O2) / _O3f;
                    break;
                }
                // divide immediate value by register               fdivir    (dest)    (a)        1.0        dest = 1.0 / a
                case vmi::fdivir: {
                    GRf(_O1) = _O3f / GRf(_O2);
                    break;
                }
                // add two registers                                add        (dest)    (a)        (b)        dest = a + b
                case vmi::dadd: {
                    GRd(_O1) = GRd(_O2) + GRd(_O3);
                    break;
                }
                // add register and immediate value                 faddi    (dest)    (a)        1.0        dest = a + 1.0
                case vmi::daddi: {
                    GRd(_O1) = GRd(_O2) + _O3d;
                    break;
                }
                // subtract register from another                   fsub    (dest)    (a)        (b)        dest = a - b
                case vmi::dsub: {
                    GRd(_O1) = GRd(_O2) - GRd(_O3);
                    break;
                }
                // subtract immediate value from register           fsubi    (dest)    (a)        1.0        dest = a - 1.0
                case vmi::dsubi: {
                    GRd(_O1) = GRd(_O2) - _O3d;
                    break;
                }
                // subtract register from immediate value           fsubir    (dest)    (a)        1.0        dest = 1.0 - a
                case vmi::dsubir: {
                    GRd(_O1) = _O3d - GRd(_O2);
                    break;
                }
                // multiply two registers                           fmul    (dest)    (a)        (b)        dest = a * b
                case vmi::dmul: {
                    GRd(_O1) = GRd(_O2) * GRd(_O3);
                    break;
                }
                // multiply register and immediate value            fmuli    (dest)    (a)        1.0        dest = a * 1.0
                case vmi::dmuli: {
                    GRd(_O1) = GRd(_O2) * _O3d;
                    break;
                }
                // divide register by another                       fdiv    (dest)    (a)        (b)        dest = a / b
                case vmi::ddiv: {
                    GRd(_O1) = GRd(_O2) / GRd(_O3);
                    break;
                }
                // divide register by immediate value               fdivi    (dest)    (a)        1.0        dest = a / 1.0
                case vmi::ddivi: {
                    GRd(_O1) = GRd(_O2) / _O3d;
                    break;
                }
                // divide immediate value by register               fdivir    (dest)    (a)        1.0        dest = 1.0 / a
                case vmi::ddivir: {
                    GRd(_O1) = _O3d / GRd(_O2);
                    break;
                }
                // logical and                                      and        (dest)    (a)        (b)        dest = a && b
                case vmi::and: {
                    GR64(_O1) = GR64(_O2) && GR64(_O3);
                    break;
                }
                // logical or                                       or        (dest)    (a)        (b)        dest = a || b
                case vmi::or: {
                    GR64(_O1) = GR64(_O2) || GR64(_O3);
                    break;
                }
                // bitwise and                                      band    (dest)    (a)        (b)        dest = a & b
                case vmi::band: {
                    GR64(_O1) = GR64(_O2) & GR64(_O3);
                    break;
                }
                // bitwise and register and immediate value         bandi    (dest)    (a)        0x0F    dest = a & 0x0F
                case vmi::bandi: {
                    GR64(_O1) = GR64(_O2) & _O3i;
                    break;
                }
                // bitwise or                                       bor        (dest)    (a)        (b)        dest = a | b
                case vmi::bor: {
                    GR64(_O1) = GR64(_O2) | GR64(_O3);
                    break;
                }
                // bitwise or register and immediate value          bori    (dest)    (a)        0x0F    dest = a | 0x0F
                case vmi::bori: {
                    GR64(_O1) = GR64(_O2) | _O3i;
                    break;
                }
                // exclusive or                                     xor        (dest)    (a)        (b)        dest = a ^ b
                case vmi::xor: {
                    GR64(_O1) = GR64(_O2) ^ GR64(_O3);
                    break;
                }
                // exlusive or register and immediate value         xori    (dest)    (a)        0x0F    dest = a ^ 0x0F
                case vmi::xori: {
                    GR64(_O1) = GR64(_O2) ^ _O3i;
                    break;
                }
                // shift bits left by amount from register          sl        (dest)    (a)        (b)        dest = a << b
                case vmi::sl: {
                    GR64(_O1) = GR64(_O2) << GR64(_O3);
                    break;
                }
                // shift bits left by immediate value               sli        (dest)    (a)        4        dest = a << 4
                case vmi::sli: {
                    GR64(_O1) = GR64(_O2) << _O3i;
                    break;
                }
                // shift bits of immediate left by register value   slir    (dest)    (a)        4        dest = 4 << a
                case vmi::slir: {
                    GR64(_O1) = _O3i << GR64(_O2);
                    break;
                }
                // shift bits right by amount from register         sr        (dest)    (a)        (b)        dest = a >> b
                case vmi::sr: {
                    GR64(_O1) = GR64(_O2) >> GR64(_O3);
                    break;
                }
                // shift bits right by immediate value              sri        (dest)    (a)        4        dest = a >> 4
                case vmi::sri: {
                    GR64(_O1) = GR64(_O2) >> _O3i;
                    break;
                }
                // shift bits of immediate right by register value  sri        (dest)    (a)        4        dest = 4 >> a
                case vmi::srir: {
                    GR64(_O1) = _O3i >> GR64(_O2);
                    break;
                }
                // check if register less than register             lt        (dest)    (a)        (b)        dest = a < b
                case vmi::lt: {
                    GR64(_O1) = GRi(_O2) < GRi(_O3);
                    break;
                }
                // check if register less than immediate            lti        (dest)    (a)        1        dest = a < 1
                case vmi::lti: {
                    GR64(_O1) = GRi(_O2) < _O3i;
                    break;
                }
                // check if register less than or equal register    lte        (dest)    (a)        (b)        dest = a <= b
                case vmi::lte: {
                    GR64(_O1) = GRi(_O2) <= GRi(_O3);
                    break;
                }
                // check if register less than or equal immediate   ltei    (dest)    (a)        1        dest = a <= 1
                case vmi::ltei: {
                    GR64(_O1) = GRi(_O2) <= _O3i;
                    break;
                }
                // check if register greater than register          gt        (dest)    (a)        (b)        dest = a > b
                case vmi::gt: {
                    GR64(_O1) = GRi(_O2) > GRi(_O3);
                    break;
                }
                // check if register greater than immediate         gti        (dest)    (a)        1        dest = a > 1
                case vmi::gti: {
                    GR64(_O1) = GRi(_O2) > _O3i;
                    break;
                }
                // check if register greater than or equal register gte        (dest)    (a)        (b)        dest = a >= b
                case vmi::gte: {
                    GR64(_O1) = GRi(_O2) >= GRi(_O3);
                    break;
                }
                // check if register greater than or equal imm.     gtei    (dest)    (a)        1        dest = a >= 1
                case vmi::gtei: {
                    GR64(_O1) = GRi(_O2) >= _O3i;
                    break;
                }
                // check if register equal register                 cmp        (dest)    (a)        (b)        dest = a == b
                case vmi::cmp: {
                    GR64(_O1) = GRi(_O2) == GRi(_O3);
                    break;
                }
                // check if register equal immediate                cmpi    (dest)    (a)        1        dest = a == 1
                case vmi::cmpi: {
                    GR64(_O1) = GRi(_O2) == _O3i;
                    break;
                }
                // check if register not equal register             ncmp    (dest)    (a)        (b)        dest = a != b
                case vmi::ncmp: {
                    GR64(_O1) = GRi(_O2) != GRi(_O3);
                    break;
                }
                // check if register not equal immediate            ncmpi    (dest)    (a)        1        dest = a != 1
                case vmi::ncmpi: {
                    GR64(_O1) = GRi(_O2) != _O3i;
                    break;
                }
                // check if register less than register             flt        (dest)    (a)        (b)        dest = a < b
                case vmi::flt: {
                    GR64(_O1) = GRf(_O2) < GRf(_O3);
                    break;
                }
                // check if register less than immediate            flti    (dest)    (a)        1.0        dest = a < 1.0
                case vmi::flti: {
                    GR64(_O1) = GRf(_O2) < _O3f;
                    break;
                }
                // check if register less than or equal register    flte    (dest)    (a)        (b)        dest = a <= b
                case vmi::flte: {
                    GR64(_O1) = GRf(_O2) <= GRf(_O3);
                    break;
                }
                // check if register less than or equal immediate   fltei    (dest)    (a)        1.0        dest = a <= 1.0
                case vmi::fltei: {
                    GR64(_O1) = GRf(_O2) <= _O3f;
                    break;
                }
                // check if register greater than register          fgt        (dest)    (a)        (b)        dest = a > b
                case vmi::fgt: {
                    GR64(_O1) = GRf(_O2) > GRf(_O3);
                    break;
                }
                // check if register greater than immediate         fgti    (dest)    (a)        1.0        dest = a > 1.0
                case vmi::fgti: {
                    GR64(_O1) = GRf(_O2) > _O3f;
                    break;
                }
                // check if register greater than or equal register fgte    (dest)    (a)        (b)        dest = a >= b
                case vmi::fgte: {
                    GR64(_O1) = GRf(_O2) >= GRf(_O3);
                    break;
                }
                // check if register greater than or equal imm.     fgtei    (dest)    (a)        1.0        dest = a >= 1.0
                case vmi::fgtei: {
                    GR64(_O1) = GRf(_O2) >= _O3f;
                    break;
                }
                // check if register equal register                 fcmp    (dest)    (a)        (b)        dest = a == b
                case vmi::fcmp: {
                    GR64(_O1) = GRf(_O2) == GRf(_O3);
                    break;
                }
                // check if register equal immediate                fcmpi    (dest)    (a)        1.0        dest = a == 1.0
                case vmi::fcmpi: {
                    GR64(_O1) = GRf(_O2) == _O3f;
                    break;
                }
                // check if register not equal register             fncmp    (dest)    (a)        (b)        dest = a != b
                case vmi::fncmp: {
                    GR64(_O1) = GRf(_O2) != GRf(_O3);
                    break;
                }
                // check if register not equal immediate            fncmpi    (dest)    (a)        1.0        dest = a != 1.0
                case vmi::fncmpi: {
                    GR64(_O1) = GRf(_O2) != _O3f;
                    break;
                }
                // check if register less than register             flt        (dest)    (a)        (b)        dest = a < b
                case vmi::dlt: {
                    GR64(_O1) = GRd(_O2) < GRd(_O3);
                    break;
                }
                // check if register less than immediate            flti    (dest)    (a)        1.0        dest = a < 1.0
                case vmi::dlti: {
                    GR64(_O1) = GRd(_O2) < _O3d;
                    break;
                }
                // check if register less than or equal register    flte    (dest)    (a)        (b)        dest = a <= b
                case vmi::dlte: {
                    GR64(_O1) = GRd(_O2) <= GRd(_O3);
                    break;
                }
                // check if register less than or equal immediate   fltei    (dest)    (a)        1.0        dest = a <= 1.0
                case vmi::dltei: {
                    GR64(_O1) = GRd(_O2) <= _O3d;
                    break;
                }
                // check if register greater than register          fgt        (dest)    (a)        (b)        dest = a > b
                case vmi::dgt: {
                    GR64(_O1) = GRd(_O2) > GRd(_O3);
                    break;
                }
                // check if register greater than immediate         fgti    (dest)    (a)        1.0        dest = a > 1.0
                case vmi::dgti: {
                    GR64(_O1) = GRd(_O2) > _O3d;
                    break;
                }
                // check if register greater than or equal register fgte    (dest)    (a)        (b)        dest = a >= b
                case vmi::dgte: {
                    GR64(_O1) = GRd(_O2) >= GRd(_O3);
                    break;
                }
                // check if register greater than or equal imm.     fgtei    (dest)    (a)        1.0        dest = a >= 1.0
                case vmi::dgtei: {
                    GR64(_O1) = GRd(_O2) >= _O3d;
                    break;
                }
                // check if register equal register                 fcmp    (dest)    (a)        (b)        dest = a == b
                case vmi::dcmp: {
                    GR64(_O1) = GRd(_O2) == GRd(_O3);
                    break;
                }
                // check if register equal immediate                fcmpi    (dest)    (a)        1.0        dest = a == 1.0
                case vmi::dcmpi: {
                    GR64(_O1) = GRd(_O2) == _O3d;
                    break;
                }
                // check if register not equal register             fncmp    (dest)    (a)        (b)        dest = a != b
                case vmi::dncmp: {
                    GR64(_O1) = GRd(_O2) != GRd(_O3);
                    break;
                }
                // check if register not equal immediate            fncmpi    (dest)    (a)        1.0        dest = a != 1.0
                case vmi::dncmpi: {
                    GR64(_O1) = GRd(_O2) != _O3d;
                    break;
                }
                // branch if register equals zero                   beqz    (a)        (fail_addr)        if a: goto fail_addr
                case vmi::beqz: {
                    if(GRi(_O1)) *ip = _O2ui - 1;
                    break;
                }
                // branch if register not equals zero               bneqz    (a)        (fail_addr)        if !a: goto fail_addr
                case vmi::bneqz: {
                    if(!GRi(_O1)) *ip = _O2ui - 1;
                    break;
                }
                // branch if register greater than zero             bgtz    (a)        (fail_addr)        if a <= 0: goto fail_addr
                case vmi::bgtz: {
                    if(GRi(_O1) <= 0) *ip = _O2ui - 1;
                    break;
                }
                // branch if register greater than or equals zero   bgtez    (a)        (fail_addr)        if a < 0: goto fail_addr
                case vmi::bgtez: {
                    if(GRi(_O1) < 0) *ip = _O2ui - 1;
                    break;
                }
                // branch if register less than zero                bltz    (a)        (fail_addr)        if a >= 0: goto fail_addr
                case vmi::bltz: {
                    if(GRi(_O1) >= 0) *ip = _O2ui - 1;
                    break;
                }
                // branch if register less than or equals zero      bltez    (a)        (fail_addr)        if a > 0: goto fail_addr
                case vmi::bltez: {
                    if(GRi(_O1) > 0) *ip = _O2ui - 1;
                    break;
                }
                // jump to address                                  jmp        0x123                    $ip = 0x123
                case vmi::jmp: {
                    *ip = _O1ui - 1;
                    break;
                }
                // jump to address in register                      jmp        (a)                        $ip = a
                case vmi::jmpr: {
                    *ip = GRx(_O1, u64) - 1;
                    break;
                }
                // jump to address and store $ip in $ra             jal        0x123                    $ra = $ip + 1;$ip = 0x123
                case vmi::jal: {
                    u64 addr = _O1ui64;
                    if (addr < code.size()) {
                        GRx(vmr::ra, u64) = (*ip) + 1;
                        *ip = addr - 1;
                    } else call_external(addr);
                    break;
                }
                // jump to address in register and store $ip in $ra jalr    (a)                        $ra = $ip + 1;$ip = a
                case vmi::jalr: {
                    GRx(vmr::ra, u64) = (*ip) + 1;
                    *ip = GRx(_O1, u64) - 1;
                    break;
                }
                case vmi::instruction_count: {
                    throw vm_exception(m_ctx, "Invalid Instruction");
                    break;
                }
                default: {
                    throw vm_exception(m_ctx, "Invalid Instruction");
                    // deinitialize?
                    break;
                }
            }

            (*ip)++;
        }
    }

    void vm::call_external(u64 addr) {
        script_function* f = m_ctx->context()->function(addr);
        if (!f) {
            throw vm_exception(m_ctx, format("Function at 0x%lX not found", addr));
        }

        std::vector<void*> args;
        for (u8 a = 0;a < f->signature.arg_locs.size();a++) {
            script_type* tp = f->signature.arg_types[a];
            u64* reg = &(m_ctx->state()->registers[(u8)f->signature.arg_locs[a]]);
            bool is_ptr = f->access.wrapped->arg_is_ptr[a];

            if (f->signature.is_subtype_obj_ctor && a == 1) {
                // arg refers to a module/type id

                script_type* tp = m_ctx->context()->module(extract_left_u32(*reg))->types()->get(extract_right_u32(*reg));
                args.push_back(tp);
                continue;
            }

            if (tp->name == "subtype") {
                // get subtype from $v3
                u64 v3 = m_ctx->state()->registers[(u8)vmr::v3];
                script_type* tp = m_ctx->context()->module(extract_left_u32(v3))->types()->get(extract_right_u32(v3));
                if (!tp) {
                    throw vm_exception(m_ctx, format(
                        "Function '%s' is a method of a sub-type class but no type ID was provided. This is not a user error",
                        f->name.c_str()
                    ));
                }
            }

            if (tp->is_primitive) {
                // *reg is some primitive value (integer, decimal, ...)
                if (is_ptr) {
                    args.push_back(reg);
                } else {
                    args.push_back(reinterpret_cast<void*>(*reg));
                }
            } else {
                // *reg is a pointer to some structure
                if (is_ptr) {
                    args.push_back(reinterpret_cast<void*>(*reg));
                } else {
                    throw vm_exception(m_ctx, format(
                        "Function '%s' accepts type '%s' as a pass-by-value parameter. This is unsupported, please change it to a pointer or reference type to make it work",
                        f->name.c_str(), tp->name.c_str()
                    ));
                }
            }
        }

        void* ret_addr = nullptr;
        if (f->signature.return_type->size > 0) {
            if (f->signature.returns_on_stack) {
                // return_loc register contains a pointer to a memory location within the stack
                ret_addr = (void*)m_ctx->state()->registers[(u8)f->signature.return_loc];
            } else {
                // return_loc register will directly contain the return value
                ret_addr = &(m_ctx->state()->registers[(u8)f->signature.return_loc]);

                // make sure there are no left over bits or bytes from the previous value (if it's smaller than 64 bits, there would be)
                (*(u64*)ret_addr) = 0;
            }
        }

        f->access.wrapped->call(m_ctx->context()->call_vm(), ret_addr, args.data());
    }
};