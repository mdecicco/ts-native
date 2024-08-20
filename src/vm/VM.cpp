#include <tsn/vm/VM.h>
#include <tsn/vm/State.h>
#include <tsn/vm/Instruction.h>
#include <tsn/vm/VMBackend.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.hpp>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Closure.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/bind/call_common.hpp>
#include <tsn/utils/ModuleSource.h>

#include <ffi.h>
#include <utils/Array.hpp>
#include <utils/Math.hpp>

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
                    fflush(stdout);
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
                            throw "VM Stack Overflow";
                        }
                        u8* ptr = (u8*)offset;
                        GR64(_O1) = *(u8*)ptr;
                        break;
                    }
                    case vmi::ld16: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u16* ptr = (u16*)offset;
                        GR64(_O1) = *(u16*)ptr;
                        break;
                    }
                    case vmi::ld32: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u32* ptr = (u32*)offset;
                        GR64(_O1) = *(u32*)ptr;
                        break;
                    }
                    case vmi::ld64: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u64* ptr = (u64*)offset;
                        GR64(_O1) = *(u64*)ptr;
                        break;
                    }
                    case vmi::st8: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u8* ptr = (u8*)offset;
                        *ptr = GR8(_O1);
                        break;
                    }
                    case vmi::st16: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u16* ptr = (u16*)offset;
                        *ptr = GR16(_O1);
                        break;
                    }
                    case vmi::st32: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u32* ptr = (u32*)offset;
                        *ptr = GR32(_O1);
                        break;
                    }
                    case vmi::st64: {
                        u64 offset = GR64(u64(_O2)) + _O3ui64;
                        if (offset >= stack_padding_start && offset <= stack_padding_end) {
                            throw "VM Stack Overflow";
                        }
                        u64* ptr = (u64*)offset;
                        *ptr = GR64(_O1);
                        break;
                    }
                    case vmi::mptr: {
                        Module* mod = m_ctx->getModule((u32)GRx(vmr::v3, u64));
                        if (!mod) throw "VM: mptr provided invalid module ID";
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
                    case vmi::v2fset: {
                        *GRx(_O1, u64*) = *GRx(_O2, u64*);
                        break;
                    }
                    case vmi::v2fsets: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = v[1] = GRf(_O2);
                        break;
                    }
                    case vmi::v2fsetsi: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = v[1] = (f32)i.imm_f();
                        break;
                    }
                    case vmi::v2dset: {
                        f64* vd = GRx(_O1, f64*);
                        f64* vs = GRx(_O2, f64*);
                        vd[0] = vs[0];
                        vd[1] = vs[1];
                        break;
                    }
                    case vmi::v2dsets: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = v[1] = GRd(_O2);
                        break;
                    }
                    case vmi::v2dsetsi: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = v[1] = i.imm_f();
                        break;
                    }
                    case vmi::v2fadd: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] += b[0];
                        a[1] += b[1];
                        break;
                    }
                    case vmi::v2fadds: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] += s;
                        v[1] += s;
                        break;
                    }
                    case vmi::v2faddsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        break;
                    }
                    case vmi::v2dadd: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] += b[0];
                        a[1] += b[1];
                        break;
                    }
                    case vmi::v2dadds: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] += s;
                        v[1] += s;
                        break;
                    }
                    case vmi::v2daddsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        break;
                    }
                    case vmi::v2fsub: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        break;
                    }
                    case vmi::v2fsubs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        break;
                    }
                    case vmi::v2fsubsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        break;
                    }
                    case vmi::v2dsub: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        break;
                    }
                    case vmi::v2dsubs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        break;
                    }
                    case vmi::v2dsubsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        break;
                    }
                    case vmi::v2fmul: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        break;
                    }
                    case vmi::v2fmuls: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        break;
                    }
                    case vmi::v2fmulsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        break;
                    }
                    case vmi::v2dmul: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        break;
                    }
                    case vmi::v2dmuls: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        break;
                    }
                    case vmi::v2dmulsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        break;
                    }
                    case vmi::v2fdiv: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        break;
                    }
                    case vmi::v2fdivs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        break;
                    }
                    case vmi::v2fdivsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        break;
                    }
                    case vmi::v2ddiv: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        break;
                    }
                    case vmi::v2ddivs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        break;
                    }
                    case vmi::v2ddivsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        break;
                    }
                    case vmi::v2fmod: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] = fmodf(a[0], b[0]);
                        a[1] = fmodf(a[1], b[1]);
                        break;
                    }
                    case vmi::v2fmods: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        break;
                    }
                    case vmi::v2fmodsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        break;
                    }
                    case vmi::v2dmod: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] = fmod(a[0], b[0]);
                        a[1] = fmod(a[1], b[1]);
                        break;
                    }
                    case vmi::v2dmods: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        break;
                    }
                    case vmi::v2dmodsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        break;
                    }
                    case vmi::v2fneg: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        break;
                    }
                    case vmi::v2dneg: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        break;
                    }
                    case vmi::v2fdot: {
                        utils::vec2f& a = *GRx(_O2, utils::vec2f*);
                        utils::vec2f& b = *GRx(_O3, utils::vec2f*);
                        GRf(_O1) = (a.x * b.x) + (a.y * b.y);
                        break;
                    }
                    case vmi::v2ddot: {
                        f64* a = GRx(_O2, f64*);
                        f64* b = GRx(_O3, f64*);
                        GRd(_O1) = (a[0] * b[0]) + (a[1] * b[1]);
                        break;
                    }
                    case vmi::v2fmag: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = sqrtf((a[0] * a[0]) + (a[1] * a[1]));
                        break;
                    }
                    case vmi::v2dmag: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = sqrt((a[0] * a[0]) + (a[1] * a[1]));
                        break;
                    }
                    case vmi::v2fmagsq: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = (a[0] * a[0]) + (a[1] * a[1]);
                        break;
                    }
                    case vmi::v2dmagsq: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = (a[0] * a[0]) + (a[1] * a[1]);
                        break;
                    }
                    case vmi::v2fnorm: {
                        f32* v = GRx(_O1, f32*);
                        f32 ilen = 1.0f / sqrtf((v[0] * v[0]) + (v[1] * v[1]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        break;
                    }
                    case vmi::v2dnorm: {
                        f64* v = GRx(_O1, f64*);
                        f64 ilen = 1.0 / sqrt((v[0] * v[0]) + (v[1] * v[1]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        break;
                    }
                    case vmi::v3fset: {
                        f32* vd = GRx(_O1, f32*);
                        f32* vs = GRx(_O2, f32*);
                        vd[0] = vs[0];
                        vd[1] = vs[1];
                        vd[2] = vs[2];
                        break;
                    }
                    case vmi::v3fsets: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = v[1] = v[2] = GRf(_O2);
                        break;
                    }
                    case vmi::v3fsetsi: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = v[1] = v[2] = (f32)i.imm_f();
                        break;
                    }
                    case vmi::v3dset: {
                        f64* vd = GRx(_O1, f64*);
                        f64* vs = GRx(_O2, f64*);
                        vd[0] = vs[0];
                        vd[1] = vs[1];
                        vd[2] = vs[2];
                        break;
                    }
                    case vmi::v3dsets: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = v[1] = v[2] = GRd(_O2);
                        break;
                    }
                    case vmi::v3dsetsi: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = v[1] = v[2] = i.imm_f();
                        break;
                    }
                    case vmi::v3fadd: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] += b[0];
                        a[1] += b[1];
                        a[2] += b[2];
                        break;
                    }
                    case vmi::v3fadds: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        break;
                    }
                    case vmi::v3faddsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        break;
                    }
                    case vmi::v3dadd: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] += b[0];
                        a[1] += b[1];
                        a[2] += b[2];
                        break;
                    }
                    case vmi::v3dadds: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        break;
                    }
                    case vmi::v3daddsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        break;
                    }
                    case vmi::v3fsub: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        a[2] -= b[2];
                        break;
                    }
                    case vmi::v3fsubs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        break;
                    }
                    case vmi::v3fsubsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        break;
                    }
                    case vmi::v3dsub: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        a[2] -= b[2];
                        break;
                    }
                    case vmi::v3dsubs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        break;
                    }
                    case vmi::v3dsubsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        break;
                    }
                    case vmi::v3fmul: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        a[2] *= b[2];
                        break;
                    }
                    case vmi::v3fmuls: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        break;
                    }
                    case vmi::v3fmulsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        break;
                    }
                    case vmi::v3dmul: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        a[2] *= b[2];
                        break;
                    }
                    case vmi::v3dmuls: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        break;
                    }
                    case vmi::v3dmulsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        break;
                    }
                    case vmi::v3fdiv: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        a[2] /= b[2];
                        break;
                    }
                    case vmi::v3fdivs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        break;
                    }
                    case vmi::v3fdivsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        break;
                    }
                    case vmi::v3ddiv: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        a[2] /= b[2];
                        break;
                    }
                    case vmi::v3ddivs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        break;
                    }
                    case vmi::v3ddivsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        break;
                    }
                    case vmi::v3fmod: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] = fmodf(a[0], b[0]);
                        a[1] = fmodf(a[1], b[1]);
                        a[2] = fmodf(a[2], b[2]);
                        break;
                    }
                    case vmi::v3fmods: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        v[2] = fmodf(v[2], s);
                        break;
                    }
                    case vmi::v3fmodsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        v[2] = fmodf(v[2], s);
                        break;
                    }
                    case vmi::v3dmod: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] = fmod(a[0], b[0]);
                        a[1] = fmod(a[1], b[1]);
                        a[2] = fmod(a[2], b[2]);
                        break;
                    }
                    case vmi::v3dmods: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        v[2] = fmod(v[2], s);
                        break;
                    }
                    case vmi::v3dmodsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        v[2] = fmod(v[2], s);
                        break;
                    }
                    case vmi::v3fneg: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        v[2] = -v[2];
                        break;
                    }
                    case vmi::v3dneg: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        v[2] = -v[2];
                        break;
                    }
                    case vmi::v3fdot: {
                        f32* a = GRx(_O2, f32*);
                        f32* b = GRx(_O3, f32*);
                        GRf(_O1) = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
                        break;
                    }
                    case vmi::v3ddot: {
                        f64* a = GRx(_O2, f64*);
                        f64* b = GRx(_O3, f64*);
                        GRd(_O1) = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
                        break;
                    }
                    case vmi::v3fmag: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = sqrtf((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]));
                        break;
                    }
                    case vmi::v3dmag: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = sqrt((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]));
                        break;
                    }
                    case vmi::v3fmagsq: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]);
                        break;
                    }
                    case vmi::v3dmagsq: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]);
                        break;
                    }
                    case vmi::v3fnorm: {
                        f32* v = GRx(_O1, f32*);
                        f32 ilen = 1.0f / sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        v[2] *= ilen;
                        break;
                    }
                    case vmi::v3dnorm: {
                        f64* v = GRx(_O1, f64*);
                        f64 ilen = 1.0 / sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        v[2] *= ilen;
                        break;
                    }
                    case vmi::v3fcross: {
                        utils::vec3f& r = *GRx(_O1, utils::vec3f*);
                        utils::vec3f& a = *GRx(_O2, utils::vec3f*);
                        utils::vec3f& b = *GRx(_O3, utils::vec3f*);
                        r.x = a.y * b.z - a.z * b.y;
                        r.y = a.z * b.x - a.x * b.z;
                        r.z = a.x * b.y - a.y * b.x;
                        break;
                    }
                    case vmi::v3dcross: {
                        utils::vec3d& r = *GRx(_O1, utils::vec3d*);
                        utils::vec3d& a = *GRx(_O2, utils::vec3d*);
                        utils::vec3d& b = *GRx(_O3, utils::vec3d*);
                        r.x = a.y * b.z - a.z * b.y;
                        r.y = a.z * b.x - a.x * b.z;
                        r.z = a.x * b.y - a.y * b.x;
                        break;
                    }
                    case vmi::v4fadd: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] += b[0];
                        a[1] += b[1];
                        a[2] += b[2];
                        a[3] += b[3];
                        break;
                    }
                    case vmi::v4fadds: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        v[3] += s;
                        break;
                    }
                    case vmi::v4faddsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        v[3] += s;
                        break;
                    }
                    case vmi::v4dadd: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] += b[0];
                        a[1] += b[1];
                        a[2] += b[2];
                        a[3] += b[3];
                        break;
                    }
                    case vmi::v4dadds: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        v[3] += s;
                        break;
                    }
                    case vmi::v4daddsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] += s;
                        v[1] += s;
                        v[2] += s;
                        v[3] += s;
                        break;
                    }
                    case vmi::v4fsub: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        a[2] -= b[2];
                        a[3] -= b[3];
                        break;
                    }
                    case vmi::v4fsubs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        v[3] -= s;
                        break;
                    }
                    case vmi::v4fsubsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        v[3] -= s;
                        break;
                    }
                    case vmi::v4dsub: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] -= b[0];
                        a[1] -= b[1];
                        a[2] -= b[2];
                        a[3] -= b[3];
                        break;
                    }
                    case vmi::v4dsubs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        v[3] -= s;
                        break;
                    }
                    case vmi::v4dsubsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] -= s;
                        v[1] -= s;
                        v[2] -= s;
                        v[3] -= s;
                        break;
                    }
                    case vmi::v4fmul: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        a[2] *= b[2];
                        a[3] *= b[3];
                        break;
                    }
                    case vmi::v4fmuls: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        v[3] *= s;
                        break;
                    }
                    case vmi::v4fmulsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        v[3] *= s;
                        break;
                    }
                    case vmi::v4dmul: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] *= b[0];
                        a[1] *= b[1];
                        a[2] *= b[2];
                        a[3] *= b[3];
                        break;
                    }
                    case vmi::v4dmuls: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        v[3] *= s;
                        break;
                    }
                    case vmi::v4dmulsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] *= s;
                        v[1] *= s;
                        v[2] *= s;
                        v[3] *= s;
                        break;
                    }
                    case vmi::v4fdiv: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        a[2] /= b[2];
                        a[3] /= b[3];
                        break;
                    }
                    case vmi::v4fdivs: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        v[3] /= s;
                        break;
                    }
                    case vmi::v4fdivsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        v[3] /= s;
                        break;
                    }
                    case vmi::v4ddiv: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] /= b[0];
                        a[1] /= b[1];
                        a[2] /= b[2];
                        a[3] /= b[3];
                        break;
                    }
                    case vmi::v4ddivs: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        v[3] /= s;
                        break;
                    }
                    case vmi::v4ddivsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] /= s;
                        v[1] /= s;
                        v[2] /= s;
                        v[3] /= s;
                        break;
                    }
                    case vmi::v4fmod: {
                        f32* a = GRx(_O1, f32*);
                        f32* b = GRx(_O2, f32*);
                        a[0] = fmodf(a[0], b[0]);
                        a[1] = fmodf(a[1], b[1]);
                        a[2] = fmodf(a[2], b[2]);
                        a[3] = fmodf(a[3], b[3]);
                        break;
                    }
                    case vmi::v4fmods: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = GRf(_O2);
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        v[2] = fmodf(v[2], s);
                        v[3] = fmodf(v[3], s);
                        break;
                    }
                    case vmi::v4fmodsi: {
                        f32* v = GRx(_O1, f32*);
                        f32 s = (f32)i.imm_f();
                        v[0] = fmodf(v[0], s);
                        v[1] = fmodf(v[1], s);
                        v[2] = fmodf(v[2], s);
                        v[3] = fmodf(v[3], s);
                        break;
                    }
                    case vmi::v4dmod: {
                        f64* a = GRx(_O1, f64*);
                        f64* b = GRx(_O2, f64*);
                        a[0] = fmod(a[0], b[0]);
                        a[1] = fmod(a[1], b[1]);
                        a[2] = fmod(a[2], b[2]);
                        a[3] = fmod(a[3], b[3]);
                        break;
                    }
                    case vmi::v4dmods: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = GRd(_O2);
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        v[2] = fmod(v[2], s);
                        v[3] = fmod(v[3], s);
                        break;
                    }
                    case vmi::v4dmodsi: {
                        f64* v = GRx(_O1, f64*);
                        f64 s = i.imm_f();
                        v[0] = fmod(v[0], s);
                        v[1] = fmod(v[1], s);
                        v[2] = fmod(v[2], s);
                        v[3] = fmod(v[3], s);
                        break;
                    }
                    case vmi::v4fneg: {
                        f32* v = GRx(_O1, f32*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        v[2] = -v[2];
                        v[3] = -v[3];
                        break;
                    }
                    case vmi::v4dneg: {
                        f64* v = GRx(_O1, f64*);
                        v[0] = -v[0];
                        v[1] = -v[1];
                        v[2] = -v[2];
                        v[3] = -v[3];
                        break;
                    }
                    case vmi::v4fdot: {
                        f32* a = GRx(_O2, f32*);
                        f32* b = GRx(_O3, f32*);
                        GRf(_O1) = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
                        break;
                    }
                    case vmi::v4ddot: {
                        f64* a = GRx(_O2, f64*);
                        f64* b = GRx(_O3, f64*);
                        GRd(_O1) = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
                        break;
                    }
                    case vmi::v4fmag: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = sqrtf((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) + (a[3] * a[3]));
                        break;
                    }
                    case vmi::v4dmag: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = sqrt((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) + (a[3] * a[3]));
                        break;
                    }
                    case vmi::v4fmagsq: {
                        f32* a = GRx(_O2, f32*);
                        GRf(_O1) = (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) + (a[3] * a[3]);
                        break;
                    }
                    case vmi::v4dmagsq: {
                        f64* a = GRx(_O2, f64*);
                        GRd(_O1) = (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) + (a[3] * a[3]);
                        break;
                    }
                    case vmi::v4fnorm: {
                        f32* v = GRx(_O1, f32*);
                        f32 ilen = 1.0f / sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        v[2] *= ilen;
                        v[3] *= ilen;
                        break;
                    }
                    case vmi::v4dnorm: {
                        f64* v = GRx(_O1, f64*);
                        f64 ilen = 1.0 / sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
                        v[0] *= ilen;
                        v[1] *= ilen;
                        v[2] *= ilen;
                        v[3] *= ilen;
                        break;
                    }
                    case vmi::v4fcross: {
                        utils::vec4f& r = *GRx(_O1, utils::vec4f*);
                        utils::vec4f& a = *GRx(_O2, utils::vec4f*);
                        utils::vec4f& b = *GRx(_O3, utils::vec4f*);
                        r.x = a.y * b.z - a.z * b.y;
                        r.y = a.z * b.x - a.x * b.z;
                        r.z = a.x * b.y - a.y * b.x;
                        break;
                    }
                    case vmi::v4dcross: {
                        utils::vec4d& r = *GRx(_O1, utils::vec4d*);
                        utils::vec4d& a = *GRx(_O2, utils::vec4d*);
                        utils::vec4d& b = *GRx(_O3, utils::vec4d*);
                        r.x = a.y * b.z - a.z * b.y;
                        r.y = a.z * b.x - a.x * b.z;
                        r.z = a.x * b.y - a.y * b.x;
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
                        if (!fn) throw "VM: jal instruction provided invalid function ID";
                        if (fn->getWrapperAddress().isValid()) {
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
                        ffi::Closure* ref = GRx(_O1, ffi::Closure*);
                        if (!ref) throw "VM: Invalid callback passed to jalr";

                        ffi::Function* fn = ref->getTarget();
                        if (!fn) throw "VM: Invalid callback passed to jalr";

                        if (fn->getWrapperAddress().isValid()) {
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
                        throw "VM: invalid instruction";
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

            static void* argSpace[64];
            static void* primArgSpace[64];
            ffi_type* argTypeSpace[64];

            utils::Array<arg_location> argLocs;
            const auto& argInfo = fn->getSignature()->getArguments();
            getArgRegisters(fn->getSignature(), argLocs);
            for (u32 i = 0;i < argLocs.size();i++) {
                const auto& info = argInfo[i];
                
                arg_location& loc = argLocs[i];
                void* src = nullptr;
                if (loc.reg_id == vmr::sp) src = (void*)(state.registers[(u8)vmr::sp] + loc.stack_offset);
                else src = (void*)&state.registers[(u8)loc.reg_id];

                if (info.argType == arg_type::context_ptr) {
                    call_context* cctx = *(call_context**)src;
                    cctx->funcPtr = &fn->getAddress();

                    primArgSpace[i] = cctx;
                    argSpace[i] = &primArgSpace[i];
                    argTypeSpace[i] = &ffi_type_pointer;
                } else if (info.argType == arg_type::pointer) {
                    argSpace[i] = src;
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

            void (*callPtr)();
            fn->getWrapperAddress().get(&callPtr);
            ffi_call(&cif, callPtr, nullptr, argSpace);
        }
    };
};
