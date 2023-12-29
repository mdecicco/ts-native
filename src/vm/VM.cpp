#include <tsn/vm/VM.h>
#include <tsn/vm/State.h>
#include <tsn/vm/Instruction.h>
#include <tsn/vm/VMBackend.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Closure.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/bind/call_common.hpp>
#include <tsn/utils/ModuleSource.h>

#include <ffi.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace vm {
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

        VM::VM(Context* ctx, u32 stackSize) : IContextual(ctx), state(stackSize) {
            m_stackSize = stackSize;
            m_executionNestLevel = 0;
        }

        VM::~VM() {
        }

        bool VM::isExecuting() const {
            return m_executionNestLevel > 0;
        }

        void VM::prepareState() {
            if (m_executionNestLevel == 0) {
                state.registers[(u8)vmr::sp] = reinterpret_cast<u64>(state.stackBase);
                state.registers[(u8)vmr::ip] = 0;
                state.registers[(u8)vmr::ra] = 0;
            }

            state.push(vmr::ip);
            state.push(vmr::ra);
        }

        void VM::execute(const utils::Array<Instruction>& code, address entry) {
            m_executionNestLevel++;

            try {
                executeInternal(code, entry);
            } catch (const std::exception& e) {
                m_executionNestLevel--;
                throw e;
            }

            m_executionNestLevel--;
        }

        void VM::executeInternal(const utils::Array<Instruction>& code, address entry) {
            GR64(vmr::ip) = entry;
            GR64(vmr::ra) = 0;
            GR64(vmr::zero) = 0;

            u64 stack_padding_start = GR64(vmr::sp) + m_stackSize;
            u64 stack_padding_end = stack_padding_start + STACK_PADDING_SIZE;

            u64* ip = &GRx(vmr::ip, u64);
            u64 cs = code.size();
            bool term = false;
            const Instruction* iptr = code.data();
            iptr += *ip;
            constexpr bool debug = false;
            u32 lastLoggedLn = -1;

            while ((*ip) <= cs && !term) {
                const Instruction& i = *iptr;

                if constexpr (debug) {
                    Backend* be = (Backend*)m_ctx->getBackend();
                    const auto& map = be->getSourceMap();
                    const auto& ref = map.get((u32)*ip);
                    if (ref.line != lastLoggedLn) {
                        printf("\n\n%s\n", ref.src->getLine(ref.line).clone().c_str());
                    }
                    lastLoggedLn = ref.line;
                    
                    printf("0x%2.2llx: %s\n", *ip, i.toString(m_ctx).c_str());
                    _flushall();
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
                            throw std::exception("VM Stack Overflow");
                        }
                        u8* ptr = (u8*)offset;
                        GR64(_O1) = *(u8*)ptr;
                        break;
                    }
                    case vmi::ld16: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u16* ptr = (u16*)offset;
                        GR64(_O1) = *(u16*)ptr;
                        break;
                    }
                    case vmi::ld32: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u32* ptr = (u32*)offset;
                        GR64(_O1) = *(u32*)ptr;
                        break;
                    }
                    case vmi::ld64: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u64* ptr = (u64*)offset;
                        GR64(_O1) = *(u64*)ptr;
                        break;
                    }
                    case vmi::st8: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u8* ptr = (u8*)offset;
                        *ptr = GR8(_O1);
                        break;
                    }
                    case vmi::st16: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u16* ptr = (u16*)offset;
                        *ptr = GR16(_O1);
                        break;
                    }
                    case vmi::st32: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u32* ptr = (u32*)offset;
                        *ptr = GR32(_O1);
                        break;
                    }
                    case vmi::st64: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw std::exception("VM Stack Overflow");
                        }
                        u64* ptr = (u64*)offset;
                        *ptr = GR64(_O1);
                        break;
                    }
                    case vmi::mptr: {
                        Module* mod = m_ctx->getModule((u32)GRx(vmr::v3, u64));
                        if (!mod) throw std::exception("VM: mptr provided invalid module ID");
                        GR64(_O1) = (u64)mod->getDataInfo(u32(_O2ui)).ptr;
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
                    case vmi::_and: {
                        GR64(_O1) = GR64(_O2) && GR64(_O3);
                        break;
                    }
                    case vmi::_or: {
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
                    case vmi::_xor: {
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
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::bneqz: {
                        if(!GRi(_O1)) {
                            *ip = _O2ui - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::bgtz: {
                        if(GRi(_O1) <= 0) {
                            *ip = _O2ui - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::bgtez: {
                        if(GRi(_O1) < 0) {
                            *ip = _O2ui - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::bltz: {
                        if(GRi(_O1) >= 0) {
                            *ip = _O2ui - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::bltez: {
                        if(GRi(_O1) > 0) {
                            *ip = _O2ui - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::jmp: {
                        *ip = _O1ui - 1;
                        iptr = code.data() + *ip;
                        break;
                    }
                    case vmi::jmpr: {
                        *ip = GRx(_O1, u64) - 1;
                        iptr = code.data() + *ip;
                        break;
                    }
                    case vmi::jal: {
                        function_id id = (function_id)_O1ui64;
                        ffi::Function* fn = m_ctx->getFunctions()->getFunction(id);
                        if (!fn) throw std::exception("VM: jal instruction provided invalid function ID");
                        if (fn->getAddress()) {
                            call_external(fn);
                        } else {
                            Backend* be = (Backend*)m_ctx->getBackend();
                            const auto* fd = be->getFunctionData(fn);
                            GRx(vmr::ra, u64) = (*ip) + 1;
                            *ip = ((u64)fd->begin) - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    case vmi::jalr: {
                        ffi::ClosureRef* ref = GRx(_O1, ffi::ClosureRef*);
                        if (!ref) throw std::exception("VM: Invalid callback passed to jalr");

                        ffi::Function* fn = ref->getTarget();
                        if (!fn) throw std::exception("VM: Invalid callback passed to jalr");

                        if (((u64)fn->getAddress()) > code.size()) {
                            call_external(fn);
                        } else {
                            Backend* be = (Backend*)m_ctx->getBackend();
                            const auto* fd = be->getFunctionData(fn);
                            GRx(vmr::ra, u64) = (*ip) + 1;
                            *ip = ((u64)fd->begin) - 1;
                            iptr = code.data() + *ip;
                        }
                        break;
                    }
                    default: {
                        throw std::exception("VM: invalid instruction");
                        break;
                    }
                }

                (*ip)++;
                iptr++;
            }

            state.pop(vmr::ra);
            state.pop(vmr::ip);
        }

        void VM::call_external(ffi::Function* fn) {
            ffi::DataType* voidp = m_ctx->getTypes()->getVoidPtr();

            // Actual signature:
            // If method of class
            //     RetTp (RetTp (ThisTp::*)(ArgTypes...), RetTp*, ExecutionContext*, ThisTp*, ArgsTypes...)
            // If normal function
            //     RetTp (RetTp (*)(ArgTypes...), RetTp*, ExecutionContext*, ArgsTypes...)
            void* callPtr = fn->getWrapperAddress();
            void* funcPtr = fn->getAddress();

            // callPtr must be called with arguments:
            // callPtr(funcPtr, retPtr, ectx, ...)

            static void* argSpace[64];
            static void* primArgSpace[64];
            ffi_type* argTypeSpace[64];

            utils::Array<arg_location> argLocs;
            const auto& argInfo = fn->getSignature()->getArguments();
            getArgRegisters(fn->getSignature(), argLocs);
            for (u32 i = 0;i < argLocs.size();i++) {
                const auto& info = argInfo[i];
                // only parameter that must be explicitly handled is func_ptr, because
                // until now it was undefined in the code. IR code as well as VM code
                // just pass an undefined value as a placeholder to make life easier
                // when handling function signatures.
                if (info.argType == arg_type::func_ptr) {
                    argSpace[i] = &funcPtr;
                    argTypeSpace[i] = &ffi_type_pointer;
                    continue;
                }
                
                arg_location& loc = argLocs[i];
                void* src = nullptr;
                if (loc.reg_id == vmr::sp) src = (void*)(state.registers[(u8)vmr::sp] + loc.stack_offset);
                else src = (void*)&state.registers[(u8)loc.reg_id];

                if (info.isImplicit()) {
                    auto& ti = info.dataType->getInfo();
                    if (info.argType == arg_type::this_ptr && ti.is_primitive) {
                        // pseudo method of primitive type
                        // the 'this' argument is always T*, even if T = void*
                        primArgSpace[i] = src;
                        argSpace[i] = &primArgSpace[i];
                    } else argSpace[i] = src;
                    argTypeSpace[i] = &ffi_type_pointer;
                } else if (info.argType == arg_type::pointer) {
                    auto& ti = info.dataType->getInfo();
                    if (ti.is_primitive && info.dataType != voidp) {
                        primArgSpace[i] = src;
                        argSpace[i] = &primArgSpace[i];
                    } else argSpace[i] = src;

                    argTypeSpace[i] = &ffi_type_pointer;
                } else {
                    argSpace[i] = src;
                    argTypeSpace[i] = getType(info.dataType);
                }
            }

            ffi_cif cif;
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argLocs.size(), &ffi_type_void, argTypeSpace) != FFI_OK) {
                return;
            }

            ffi_call(&cif, reinterpret_cast<void (*)()>(callPtr), nullptr, argSpace);
        }
    };
};