#include <gjs/vm/vm.h>
#include <gjs/vm/allocator.h>
#include <stdarg.h>
#include <string.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/common/function_pointer.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/backends/b_vm.h>

namespace gjs {
    #define vmi vm_instruction
    #define vmr vm_register
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

    vm::vm(vm_backend* ctx, vm_allocator* allocator, u32 stack_size, u32 mem_size) : m_ctx(ctx), alloc(allocator), state(mem_size), m_stack_size(stack_size), m_subtype_t(nullptr) {
    }

    vm::~vm() {
    }

    void vm::execute(const instruction_array& code, address entry, bool nested) {
        struct {
            u64 ip, ra;
        } prev = { 0, 0 };

        if (nested) {
            prev = { GR64(vmr::ip), GR64(vmr::ra) };
        } else {
            GR64(vmr::sp) = (u64)state.memory[0];
        }

        GR64(vmr::ip) = entry;
        GR64(vmr::ra) = 0;
        GR64(vmr::zero) = 0;

        u64 stack_padding_start = ((u64)state.memory[0]) + m_stack_size;
        u64 stack_padding_end = stack_padding_start + STACK_PADDING_SIZE;

        u64* ip = &GRx(vmr::ip, u64);
        u64 cs = code.size();
        bool term = false;
        const instruction* iptr = code.ptr();
        iptr += *ip;
        bool logInstrs = m_ctx->log_instructions();
        bool logLines = m_ctx->log_lines();
        u32 lastLoggedLn = -1;
        while ((*ip) <= cs && !term) {
            const instruction& i = *iptr;
            if (logLines) {
                source_ref ref = m_ctx->map()->get(*ip);
                if (ref.line != lastLoggedLn) printf("\n\n%s\n", ref.line_text.c_str());
                lastLoggedLn = ref.line;
            }
            if (logInstrs) {
                printf("0x%2.2llx: %s\n", *ip, i.to_string(m_ctx).c_str());
            }

            vmi instr = i.instr();
            switch (instr) {
                case vmi::null: {
                    break;
                }
                case vmi::term: {
                    term = true;
                    break;
                }
                case vmi::ld8: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u8* ptr = (u8*)offset;
                    GR64(_O1) = *(u8*)ptr;
                    break;
                }
                case vmi::ld16: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u16* ptr = (u16*)offset;
                    GR64(_O1) = *(u16*)ptr;
                    break;
                }
                case vmi::ld32: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u32* ptr = (u32*)offset;
                    GR64(_O1) = *(u32*)ptr;
                    break;
                }
                case vmi::ld64: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u64* ptr = (u64*)offset;
                    GR64(_O1) = *(u64*)ptr;
                    break;
                }
                case vmi::st8: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u8* ptr = (u8*)offset;
                    *ptr = GR8(_O1);
                    break;
                }
                case vmi::st16: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u16* ptr = (u16*)offset;
                    *ptr = GR16(_O1);
                    break;
                }
                case vmi::st32: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u32* ptr = (u32*)offset;
                    *ptr = GR32(_O1);
                    break;
                }
                case vmi::st64: {
                    u64 offset = GR64(u64(_O2)) + _O3ui64;
                    if (offset >= stack_padding_start && offset <= stack_padding_end) {
                        throw error::vm_exception(error::ecode::vm_stack_overflow);
                    }
                    u64* ptr = (u64*)offset;
                    *ptr = GR64(_O1);
                    break;
                }
                case vmi::mptr: {
                    script_module* mod = m_ctx->context()->module((u32)GRx(vmr::v3, u64));
                    if (!mod) throw error::vm_exception(error::ecode::vm_invalid_module_id);
                    GR64(_O1) = (u64)mod->data()->data(_O2ui);
                    break;
                }
                case vmi::mtfp: {
                    GR64(_O2) = GR64(_O1);
                    break;
                }
                case vmi::mffp: {
                    GR64(_O2) = GR64(_O1);
                    break;
                }
                case vmi::add: {
                    GRx(_O1, i64) = GRx(_O2, i64) + GRx(_O3, i64);
                    break;
                }
                case vmi::addi: {
                    GRx(_O1, i64) = GRx(_O2, i64) + _O3i;
                    break;
                }
                case vmi::sub: {
                    GRx(_O1, i64) = GRx(_O2, i64) - GRx(_O3, i64);
                    break;
                }
                case vmi::subi: {
                    GRx(_O1, i64) = GRx(_O2, i64) - _O3i;
                    break;
                }
                case vmi::subir: {
                    GRx(_O1, i64) = _O3i - GRx(_O2, i64);
                    break;
                }
                case vmi::mul: {
                    GRx(_O1, i64) = GRx(_O2, i64) * GRi(_O3);
                    break;
                }
                case vmi::muli: {
                    GRx(_O1, i64) = GRx(_O2, i64) * _O3i;
                    break;
                }
                case vmi::div: {
                    GRx(_O1, i64) = GRx(_O2, i64) / GRx(_O3, i64);
                    break;
                }
                case vmi::divi: {
                    GRx(_O1, i64) = GRx(_O2, i64) / _O3i;
                    break;
                }
                case vmi::divir: {
                    GRx(_O1, i64) = _O3i / GRx(_O2, i64);
                    break;
                }
                case vmi::neg: {
                    GRi(_O1) = -GRi(_O2);
                    break;
                }
                case vmi::addu: {
                    GRx(_O1, u64) = GRx(_O2, u64) + GRx(_O3, u64);
                    break;
                }
                case vmi::addui: {
                    u64* raddr = &GRx(_O1, u64);
                    GRx(_O1, u64) = GRx(_O2, u64) + _O3ui;
                    break;
                }
                case vmi::subu: {
                    GRx(_O1, u64) = GRx(_O2, u64) - GRx(_O3, u64);
                    break;
                }
                case vmi::subui: {
                    GRx(_O1, u64) = GRx(_O2, u64) - _O3ui;
                    break;
                }
                case vmi::subuir: {
                    GRx(_O1, u64) = _O3ui - GRx(_O2, u64);
                    break;
                }
                case vmi::mulu: {
                    GRx(_O1, u64) = GRx(_O2, u64) * GRx(_O3, u64);
                    break;
                }
                case vmi::mului: {
                    GRx(_O1, u64) = GRx(_O2, u64) * _O3ui;
                    break;
                }
                case vmi::divu: {
                    GRx(_O1, u64) = GRx(_O2, u64) / GRx(_O3, u64);
                    break;
                }
                case vmi::divui: {
                    GRx(_O1, u64) = GRx(_O2, u64) / _O3ui;
                    break;
                }
                case vmi::divuir: {
                    GRx(_O1, u64) = _O3ui / GRx(_O2, u64);
                    break;
                }
                case vmi::cvt_if: {
                    i64* r = &GRx(_O1, i64);
                    f32 v = (f32)*r;
                    *r = 0;
                    (*(f32*)r) = v;
                    break;
                }
                case vmi::cvt_id: {
                    i64* r = &GRx(_O1, i64);
                    (*(f64*)r) = (f64)*r;
                    break;
                }
                case vmi::cvt_iu: {
                    i64* r = &GRx(_O1, i64);
                    (*(u64*)r) = *r;
                    break;
                }
                case vmi::cvt_uf: {
                    u64* r = &GRx(_O1, u64);
                    f32 v = (f32)*r;
                    *r = 0;
                    (*(f32*)r) = v;
                    break;
                }
                case vmi::cvt_ud: {
                    u64* r = &GRx(_O1, u64);
                    (*(f64*)r) = (f64)*r;
                    break;
                }
                case vmi::cvt_ui: {
                    u64* r = &GRx(_O1, u64);
                    (*(i64*)r) = *r;
                    break;
                }
                case vmi::cvt_fi: {
                    f32* r = &GRx(_O1, f32);
                    (*(i64*)r) = (i64)*r;
                    break;
                }
                case vmi::cvt_fu: {
                    f32* r = &GRx(_O1, f32);
                    (*(u64*)r) = (u64)*r;
                    break;
                }
                case vmi::cvt_fd: {
                    f32* r = &GRx(_O1, f32);
                    (*(f64*)r) = *r;
                    break;
                }
                case vmi::cvt_di: {
                    f64* r = &GRx(_O1, f64);
                    (*(i64*)r) = (i64)*r;
                    break;
                }
                case vmi::cvt_du: {
                    f64* r = &GRx(_O1, f64);
                    (*(u64*)r) = (u64)*r;
                    break;
                }
                case vmi::cvt_df: {
                    f64* r = &GRx(_O1, f64);
                    f32 v = (f32)*r;
                    (*(u64*)r) = 0;
                    (*(f32*)r) = v;
                    break;
                }
                case vmi::fadd: {
                    GRf(_O1) = GRf(_O2) + GRf(_O3);
                    break;
                }
                case vmi::faddi: {
                    GRf(_O1) = GRf(_O2) + _O3f;
                    break;
                }
                case vmi::fsub: {
                    GRf(_O1) = GRf(_O2) - GRf(_O3);
                    break;
                }
                case vmi::fsubi: {
                    GRf(_O1) = GRf(_O2) - _O3f;
                    break;
                }
                case vmi::fsubir: {
                    GRf(_O1) = _O3f - GRf(_O2);
                    break;
                }
                case vmi::fmul: {
                    GRf(_O1) = GRf(_O2) * GRf(_O3);
                    break;
                }
                case vmi::fmuli: {
                    GRf(_O1) = GRf(_O2) * _O3f;
                    break;
                }
                case vmi::fdiv: {
                    GRf(_O1) = GRf(_O2) / GRf(_O3);
                    break;
                }
                case vmi::fdivi: {
                    GRf(_O1) = GRf(_O2) / _O3f;
                    break;
                }
                case vmi::fdivir: {
                    GRf(_O1) = _O3f / GRf(_O2);
                    break;
                }
                case vmi::negf: {
                    GRf(_O1) = -GRf(_O2);
                    break;
                }
                case vmi::dadd: {
                    GRd(_O1) = GRd(_O2) + GRd(_O3);
                    break;
                }
                case vmi::daddi: {
                    GRd(_O1) = GRd(_O2) + _O3d;
                    break;
                }
                case vmi::dsub: {
                    GRd(_O1) = GRd(_O2) - GRd(_O3);
                    break;
                }
                case vmi::dsubi: {
                    GRd(_O1) = GRd(_O2) - _O3d;
                    break;
                }
                case vmi::dsubir: {
                    GRd(_O1) = _O3d - GRd(_O2);
                    break;
                }
                case vmi::dmul: {
                    GRd(_O1) = GRd(_O2) * GRd(_O3);
                    break;
                }
                case vmi::dmuli: {
                    GRd(_O1) = GRd(_O2) * _O3d;
                    break;
                }
                case vmi::ddiv: {
                    GRd(_O1) = GRd(_O2) / GRd(_O3);
                    break;
                }
                case vmi::ddivi: {
                    GRd(_O1) = GRd(_O2) / _O3d;
                    break;
                }
                case vmi::ddivir: {
                    GRd(_O1) = _O3d / GRd(_O2);
                    break;
                }
                case vmi::negd: {
                    GRd(_O1) = -GRd(_O2);
                    break;
                }
                case vmi::and: {
                    GR64(_O1) = GR64(_O2) && GR64(_O3);
                    break;
                }
                case vmi::or: {
                    GR64(_O1) = GR64(_O2) || GR64(_O3);
                    break;
                }
                case vmi::band: {
                    GR64(_O1) = GR64(_O2) & GR64(_O3);
                    break;
                }
                case vmi::bandi: {
                    GR64(_O1) = GR64(_O2) & _O3i;
                    break;
                }
                case vmi::bor: {
                    GR64(_O1) = GR64(_O2) | GR64(_O3);
                    break;
                }
                case vmi::bori: {
                    GR64(_O1) = GR64(_O2) | _O3i;
                    break;
                }
                case vmi::xor: {
                    GR64(_O1) = GR64(_O2) ^ GR64(_O3);
                    break;
                }
                case vmi::xori: {
                    GR64(_O1) = GR64(_O2) ^ _O3i;
                    break;
                }
                case vmi::sl: {
                    GR64(_O1) = GR64(_O2) << GR64(_O3);
                    break;
                }
                case vmi::sli: {
                    GR64(_O1) = GR64(_O2) << _O3i;
                    break;
                }
                case vmi::slir: {
                    GR64(_O1) = _O3i << GR64(_O2);
                    break;
                }
                case vmi::sr: {
                    GR64(_O1) = GR64(_O2) >> GR64(_O3);
                    break;
                }
                case vmi::sri: {
                    GR64(_O1) = GR64(_O2) >> _O3i;
                    break;
                }
                case vmi::srir: {
                    GR64(_O1) = _O3i >> GR64(_O2);
                    break;
                }
                case vmi::lt: {
                    GR64(_O1) = GRi(_O2) < GRi(_O3);
                    break;
                }
                case vmi::lti: {
                    GR64(_O1) = GRi(_O2) < _O3i;
                    break;
                }
                case vmi::lte: {
                    GR64(_O1) = GRi(_O2) <= GRi(_O3);
                    break;
                }
                case vmi::ltei: {
                    GR64(_O1) = GRi(_O2) <= _O3i;
                    break;
                }
                case vmi::gt: {
                    GR64(_O1) = GRi(_O2) > GRi(_O3);
                    break;
                }
                case vmi::gti: {
                    GR64(_O1) = GRi(_O2) > _O3i;
                    break;
                }
                case vmi::gte: {
                    GR64(_O1) = GRi(_O2) >= GRi(_O3);
                    break;
                }
                case vmi::gtei: {
                    GR64(_O1) = GRi(_O2) >= _O3i;
                    break;
                }
                case vmi::cmp: {
                    GR64(_O1) = GRi(_O2) == GRi(_O3);
                    break;
                }
                case vmi::cmpi: {
                    GR64(_O1) = GRi(_O2) == _O3i;
                    break;
                }
                case vmi::ncmp: {
                    GR64(_O1) = GRi(_O2) != GRi(_O3);
                    break;
                }
                case vmi::ncmpi: {
                    GR64(_O1) = GRi(_O2) != _O3i;
                    break;
                }
                case vmi::flt: {
                    GR64(_O1) = GRf(_O2) < GRf(_O3);
                    break;
                }
                case vmi::flti: {
                    GR64(_O1) = GRf(_O2) < _O3f;
                    break;
                }
                case vmi::flte: {
                    GR64(_O1) = GRf(_O2) <= GRf(_O3);
                    break;
                }
                case vmi::fltei: {
                    GR64(_O1) = GRf(_O2) <= _O3f;
                    break;
                }
                case vmi::fgt: {
                    GR64(_O1) = GRf(_O2) > GRf(_O3);
                    break;
                }
                case vmi::fgti: {
                    GR64(_O1) = GRf(_O2) > _O3f;
                    break;
                }
                case vmi::fgte: {
                    GR64(_O1) = GRf(_O2) >= GRf(_O3);
                    break;
                }
                case vmi::fgtei: {
                    GR64(_O1) = GRf(_O2) >= _O3f;
                    break;
                }
                case vmi::fcmp: {
                    GR64(_O1) = GRf(_O2) == GRf(_O3);
                    break;
                }
                case vmi::fcmpi: {
                    GR64(_O1) = GRf(_O2) == _O3f;
                    break;
                }
                case vmi::fncmp: {
                    GR64(_O1) = GRf(_O2) != GRf(_O3);
                    break;
                }
                case vmi::fncmpi: {
                    GR64(_O1) = GRf(_O2) != _O3f;
                    break;
                }
                case vmi::dlt: {
                    GR64(_O1) = GRd(_O2) < GRd(_O3);
                    break;
                }
                case vmi::dlti: {
                    GR64(_O1) = GRd(_O2) < _O3d;
                    break;
                }
                case vmi::dlte: {
                    GR64(_O1) = GRd(_O2) <= GRd(_O3);
                    break;
                }
                case vmi::dltei: {
                    GR64(_O1) = GRd(_O2) <= _O3d;
                    break;
                }
                case vmi::dgt: {
                    GR64(_O1) = GRd(_O2) > GRd(_O3);
                    break;
                }
                case vmi::dgti: {
                    GR64(_O1) = GRd(_O2) > _O3d;
                    break;
                }
                case vmi::dgte: {
                    GR64(_O1) = GRd(_O2) >= GRd(_O3);
                    break;
                }
                case vmi::dgtei: {
                    GR64(_O1) = GRd(_O2) >= _O3d;
                    break;
                }
                case vmi::dcmp: {
                    GR64(_O1) = GRd(_O2) == GRd(_O3);
                    break;
                }
                case vmi::dcmpi: {
                    GR64(_O1) = GRd(_O2) == _O3d;
                    break;
                }
                case vmi::dncmp: {
                    GR64(_O1) = GRd(_O2) != GRd(_O3);
                    break;
                }
                case vmi::dncmpi: {
                    GR64(_O1) = GRd(_O2) != _O3d;
                    break;
                }
                case vmi::beqz: {
                    if(GRi(_O1)) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::bneqz: {
                    if(!GRi(_O1)) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::bgtz: {
                    if(GRi(_O1) <= 0) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::bgtez: {
                    if(GRi(_O1) < 0) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::bltz: {
                    if(GRi(_O1) >= 0) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::bltez: {
                    if(GRi(_O1) > 0) {
                        *ip = _O2ui - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::jmp: {
                    *ip = _O1ui - 1;
                    iptr = code.ptr() + *ip;
                    break;
                }
                case vmi::jmpr: {
                    *ip = GRx(_O1, u64) - 1;
                    iptr = code.ptr() + *ip;
                    break;
                }
                case vmi::jal: {
                    function_id id = (function_id)_O1ui64;
                    script_function* fn = m_ctx->context()->function(id);
                    if (!fn) throw error::vm_exception(error::ecode::vm_invalid_function_id);
                    if (fn->is_host) {
                        call_external(fn);
                    }
                    else {
                        GRx(vmr::ra, u64) = (*ip) + 1;
                        *ip = fn->access.entry - 1;
                        iptr = code.ptr() + *ip;
                    }
                    break;
                }
                case vmi::jalr: {
                    raw_callback** cb = GRx(_O1, raw_callback**);
                    if (!cb || !*cb) throw error::vm_exception(error::ecode::vm_invalid_callback);
                    function_pointer* fp = (*cb)->ptr;
                    if (!fp->target) throw error::vm_exception(error::ecode::vm_invalid_callback);

                    if (fp->target->is_host) {
                        call_external(cb);
                    }
                    else {
                        GRx(vmr::ra, u64) = (*ip) + 1;
                        *ip = fp->target->access.entry - 1;
                        iptr = code.ptr() + *ip;
                    }

                    break;
                }
                default: {
                    throw error::vm_exception(error::ecode::vm_invalid_instruction);
                    break;
                }
            }

            (*ip)++;
            iptr++;
        }

        GR64(vmr::ip) = prev.ip;
        GR64(vmr::ra) = prev.ra;
    }

    void vm::call_external(script_function* f) {
        if (!m_subtype_t) m_subtype_t = m_ctx->context()->type<subtype_t>();
        u8 stackReturnRegId = 0;

        void* args[16];
        u8 ac = (u8)f->type->signature->args.size();
        u64* registers = m_ctx->state()->registers;
        u8 targetArg = 0;
        for (u8 a = 0;a < ac;a++) {
            auto& arg = f->type->signature->args[a];
            u8 regId = (u8)arg.loc;
            script_type* tp = arg.tp;
            u64* reg = &(registers[regId]);

            if (arg.implicit == function_signature::argument::implicit_type::ret_addr) {
                // arg refers to output pointer which is passed to 'call' function and not to the target function directly
                stackReturnRegId = regId;
                continue;
            }

            if (tp == m_subtype_t) {
                // get subtype from $v3
                u64 v3 = registers[(u8)vmr::v3];
                tp = m_ctx->context()->module(extract_left_u32(v3))->types()->get(extract_right_u32(v3));
                if (!tp) {
                    throw error::vm_exception(error::ecode::vm_no_subtype_provided_for_call, f->name.c_str());
                }
            }

            if (tp->is_primitive && !tp->signature) {
                // *reg is some primitive value (integer, decimal, ...)
                if (arg.is_ptr) {
                    args[targetArg++] = reg;
                } else {
                    args[targetArg++] = reinterpret_cast<void*>(*reg);
                }
            } else {
                // *reg is a pointer to some structure
                if (arg.is_ptr) {
                    args[targetArg++] = reinterpret_cast<void*>(*reg);
                } else {
                    throw error::vm_exception(error::ecode::vm_cannot_pass_struct_by_value, f->name.c_str(), tp->name.c_str());
                }
            }
        }

        void* ret_addr = nullptr;
        if (f->type->signature->return_type->size > 0) {
            if (f->type->signature->returns_on_stack) {
                // implicit return pointer argument register contains a pointer to a memory location within the stack
                ret_addr = (void*)registers[stackReturnRegId];
            } else {
                // return_loc register will directly contain the return value
                ret_addr = &(registers[(u8)f->type->signature->return_loc]);

                // make sure there are no left over bits or bytes from the previous value (if it's smaller than 64 bits, there would be)
                (*(u64*)ret_addr) = 0;
            }
        }

        f->access.wrapped->call(m_ctx->context()->call_vm(), ret_addr, args);
    }

    void vm::call_external(raw_callback** cb) {
        if (!m_subtype_t) m_subtype_t = m_ctx->context()->type<subtype_t>();
        script_function* f = (*cb)->ptr->target;
        u8 stackReturnRegId = 0;

        void* args[16];
        u8 ac = (u8)f->type->signature->args.size();
        u8 targetArg = 0;
        u64* registers = m_ctx->state()->registers;
        bool hadThis = false;
        for (u8 a = 0;a < ac;a++) {
            auto& arg = f->type->signature->args[a];
            u8 regId = (u8)arg.loc;
            script_type* tp = arg.tp;

            if (arg.implicit == function_signature::argument::implicit_type::this_ptr) {
                args[targetArg++] = (*cb)->ptr->self_obj();
                hadThis = true;
                continue;
            }

            if (hadThis && vm_register(regId) >= vmr::a0 && vm_register(regId) <= vmr::a7) {
                // script code can't account for the 'this' argument of host callbacks, so
                // it passes arguments as if there is no 'this' argument passed through $a0.
                // Once the 'this' argument of the callback has been passed, all GP register
                // arguments after that must be decreased by one in order to map the actual
                // argument locations to the VM argument locations. For example, if the vm
                // calls a host callback with the signature void (int, int):
                // 
                // VM is calling signature void (int arg0: $a0, int arg1: $a1)
                // Actual signature is void (TransientFunction<void(int, int)>*: $a0, int arg0: $a1, int arg1: $a2)
                //
                // So regId here refers to the argument location of the actual signature,
                // but needs to refer to the VM provided argument location
                // This logic doesn't affect FP arguments because the 'this' argument is
                // always mapped to a GP register.
                regId--;
            }
            u64* reg = &(registers[regId]);

            if (arg.implicit == function_signature::argument::implicit_type::ret_addr) {
                // arg refers to output pointer which is passed to 'call' function and not to the target function directly
                stackReturnRegId = regId;
                continue;
            }

            if (tp == m_subtype_t) {
                // get subtype from $v3
                u64 v3 = registers[(u8)vmr::v3];
                tp = m_ctx->context()->module(extract_left_u32(v3))->types()->get(extract_right_u32(v3));
                if (!tp) {
                    throw error::vm_exception(error::ecode::vm_no_subtype_provided_for_call, f->name.c_str());
                }
            }

            if (tp->is_primitive && !tp->signature) {
                // *reg is some primitive value (integer, decimal, ...)
                if (arg.is_ptr) {
                    args[targetArg++] = reg;
                } else {
                    args[targetArg++] = reinterpret_cast<void*>(*reg);
                }
            } else {
                // *reg is a pointer to some structure
                if (arg.is_ptr) {
                    args[targetArg++] = reinterpret_cast<void*>(*reg);
                } else {
                    throw error::vm_exception(error::ecode::vm_cannot_pass_struct_by_value, f->name.c_str(), tp->name.c_str());
                }
            }
        }

        void* ret_addr = nullptr;
        if (f->type->signature->return_type->size > 0) {
            if (f->type->signature->returns_on_stack) {
                // implicit return pointer argument register contains a pointer to a memory location within the stack
                ret_addr = (void*)registers[stackReturnRegId];
            } else {
                // return_loc register will directly contain the return value
                ret_addr = &(registers[(u8)f->type->signature->return_loc]);

                // make sure there are no left over bits or bytes from the previous value (if it's smaller than 64 bits, there would be)
                (*(u64*)ret_addr) = 0;
            }
        }

        f->access.wrapped->call(m_ctx->context()->call_vm(), ret_addr, args);
    }
};