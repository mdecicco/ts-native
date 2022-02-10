#include <gjs/backends/b_x86.h>
#include <gjs/compiler/tac.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/gjs.hpp>


#include <asmjit/asmjit.h>
#include <cmath>

using namespace asmjit;

class MyErrorHandler : public ErrorHandler {
public:
    void handleError(Error err, const char* message, BaseEmitter* origin) override {
        printf("AsmJit error: %s\n", message);
    }
};

namespace gjs {
    x86_backend::x86_backend() {
        m_rt = new JitRuntime();
    }

    x86_backend::~x86_backend() {
        delete m_rt;
    }

    u32 convertType(script_type* tp) {
        if (!tp) return Type::kIdVoid;

        if (tp->is_primitive) {
            if (tp->is_floating_point) {
                if (tp->size == sizeof(f64)) return Type::kIdF64;
                return Type::kIdF32;
            } else {
                if (tp->is_unsigned) {
                    if (tp->size == sizeof(u64)) return Type::kIdU64;
                    else if (tp->size == sizeof(u32)) return Type::kIdU32;
                    else if (tp->size == sizeof(u16)) return Type::kIdU16;
                    return Type::kIdU8;
                }

                if (tp->size == sizeof(i64)) return Type::kIdI64;
                else if (tp->size == sizeof(i32)) return Type::kIdI32;
                else if (tp->size == sizeof(i16)) return Type::kIdI16;
                return Type::kIdI8;
            }
        }

        return Type::kIdUIntPtr;
    }

    Imm v2imm(const compile::var& v) {
        if (!v.is_imm()) {
            // exception
            return Imm();
        }

        if (v.type()->is_floating_point) {
            if (v.type()->size == sizeof(f64)) return Imm(v.imm_d());
            return Imm(v.imm_f());
        } else {
            if (v.type()->size == sizeof(u64)) {
                if (v.type()->is_unsigned) return Imm(v.imm_u());
                return Imm(v.imm_i());
            } else if (v.type()->size == sizeof(u32)) {
                if (v.type()->is_unsigned) return Imm((u32)v.imm_u());
                return Imm((i32)v.imm_i());
            } else if (v.type()->size == sizeof(u16)) {
                if (v.type()->is_unsigned) return Imm((u16)v.imm_u());
                return Imm((i16)v.imm_i());
            } else if (v.type()->size == sizeof(u8)) {
                if (v.type()->is_unsigned) return Imm((u8)v.imm_u());
                return Imm((i8)v.imm_i());
            }
        }

        // exception
        return Imm();
    }

    x86::Mem imm2const(x86::Compiler& cc, const compile::var& v) {
        if (!v.is_imm()) {
            // exception
            return x86::Mem();
        }

        u64 imm = v.imm_u();
        return cc.newConst(ConstPool::kScopeGlobal, &imm, v.size());
    }

    // non-lambda function signature
    void fsig(script_function* f, FuncSignatureBuilder& fsb) {
        function_signature* cfSig = f->type->signature;
        fsb.setCallConv(CallConv::kIdCDecl);

        if (cfSig->return_type->size == 0 || cfSig->returns_on_stack) fsb.setRet(Type::kIdVoid);
        else {
            if (cfSig->returns_pointer) fsb.setRet(Type::kIdUIntPtr);
            else fsb.setRet(convertType(cfSig->return_type));
        }

        // all external functions are normal cdecls
        if (f->is_host && !f->is_external) {
            if (f->access.wrapped->srv_wrapper_func) {
                if (f->access.wrapped->call_method_func) {
                    // ret ptr, call_method_func, func_ptr, call_params...
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                } else {
                    // ret ptr, func_ptr, call_params...
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                }
            } else if (f->access.wrapped->call_method_func) {
                // func_ptr, call_params
                fsb.addArg(Type::kIdUIntPtr);
            } else if (f->access.wrapped->cdecl_wrapper_func) {
                // func_ptr, call_params
                fsb.addArg(Type::kIdUIntPtr);
            } else {
                // call_params
            }
        }

        for (u8 a = 0;a < cfSig->args.size();a++) {
            if (f->is_host && cfSig->args[a].implicit == function_signature::argument::implicit_type::ret_addr) continue;
            if (cfSig->args[a].is_ptr) fsb.addArg(Type::kIdUIntPtr);
            else fsb.addArg(convertType(cfSig->args[a].tp));
        }
    }

    // Lambda function signature
    void fsig(function_signature* sig, FuncSignatureBuilder& fsb, bool is_host, bool is_srv, bool is_nonstatic_method) {
        fsb.setCallConv(CallConv::kIdCDecl);

        if (sig->return_type->size == 0 || sig->returns_on_stack) fsb.setRet(Type::kIdVoid);
        else {
            if (sig->returns_pointer) fsb.setRet(Type::kIdUIntPtr);
            else fsb.setRet(convertType(sig->return_type));
        }

        if (is_host) {
            if (is_srv) {
                if (is_nonstatic_method) {
                    // ret ptr, call_method_func, func_ptr, unused_ptr, call_params...
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                } else {
                    // ret ptr, func_ptr, unused_ptr, call_params...
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                    fsb.addArg(Type::kIdUIntPtr);
                }
            } else if (is_nonstatic_method) {
                // func_ptr, unused_ptr, call_params...
                fsb.addArg(Type::kIdUIntPtr);
                fsb.addArg(Type::kIdUIntPtr);
            } else {
                // unused_ptr, call_params...
                fsb.addArg(Type::kIdUIntPtr);
            }
        }

        for (u8 a = 0;a < sig->args.size();a++) {
            if (is_host && sig->args[a].implicit == function_signature::argument::implicit_type::ret_addr) continue;
            if (sig->args[a].is_ptr) fsb.addArg(Type::kIdUIntPtr);
            else fsb.addArg(convertType(sig->args[a].tp));
        }
    }

    x86::Reg convert(script_type* t1, script_type* t2, const compile::var& v, asmjit::x86::Compiler& cc, std::vector<x86::Reg>& args, robin_hood::unordered_map<u64, x86::Reg>& regs) {
        auto v2r = [&](const compile::var& v) -> x86::Reg {
            if (v.is_arg()) return args[v.arg_idx()];
            else if (v.is_imm()) {
                if (v.type()->is_floating_point) {
                    x86::Xmm r = cc.newXmm();
                    if (v.size() == sizeof(f64)) cc.movq(r, imm2const(cc, v));
                    else cc.movd(r, imm2const(cc, v));
                    return r;
                }
                x86::Gp r = cc.newGp(convertType(v.type()));
                cc.mov(r, imm2const(cc, v));
                return r;
            } else {
                auto it = regs.find(v.reg_id());
                if (it != regs.end()) return it->getSecond();
                if (v.type()->is_floating_point) return regs[v.reg_id()] = cc.newXmm();
                return regs[v.reg_id()] = cc.newReg(convertType(v.type()), v.name().c_str());
            }

            // exception
            return x86::Reg();
        };

        u64 a = 0x43e0000000000000;
        auto c_43e = cc.newConst(ConstPool::kScopeGlobal, &a, 8);
        u64 b = 0x43e0000000000000;
        auto c_80 = cc.newConst(ConstPool::kScopeGlobal, &b, 8);
        u32 c = 0x5f000000;
        auto c_5f = cc.newConst(ConstPool::kScopeGlobal, &c, 4);

        // good
        auto f64_to_int64 = [&](){
            x86::Gp r = cc.newInt64();
            cc.cvttsd2si(r, v2r(v).as<x86::Xmm>());
            return r;
        };
        // good
        auto f64_to_uint64 = [&](){
            x86::Gp r = cc.newUInt64();
            Label l0 = cc.newLabel();
            x86::Xmm tmp0 = cc.newXmm();
            cc.movsd(tmp0, v2r(v).as<x86::Xmm>());
            x86::Xmm tmp1 = cc.newXmm();
            cc.movsd(tmp1, c_43e);
            cc.xor_(x86::ecx, x86::ecx);
            cc.comisd(tmp0, tmp1);
            cc.jb(l0);
            cc.subsd(tmp0, tmp1);
            cc.comisd(tmp0, tmp1);
            cc.jae(l0);
            cc.mov(r, c_80);
            cc.mov(x86::rcx, r);
            cc.bind(l0);
            cc.cvttsd2si(r, tmp0);
            cc.add(r, x86::rcx);
            return r;
        };
        // good
        auto f64_to_int = [&](){
            x86::Gp r = cc.newInt32();
            cc.cvttsd2si(r, v2r(v).as<x86::Xmm>());
            return r;
        };
        // good
        auto f64_to_f32 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.cvtsd2ss(r, v2r(v).as<x86::Xmm>());
            return r;
        };
        // good
        auto f32_to_u64 = [&](){
            x86::Gp r = cc.newUInt64();
            Label l0 = cc.newLabel();
            x86::Xmm tmp0 = cc.newXmm();
            cc.movsd(tmp0, v2r(v).as<x86::Xmm>());
            x86::Xmm tmp1 = cc.newXmm();
            cc.movss(tmp1, c_5f);
            cc.xor_(x86::ecx, x86::ecx);
            cc.comiss(tmp0, tmp1);
            cc.jb(l0);
            cc.subss(tmp0, tmp1);
            cc.comiss(tmp0, tmp1);
            cc.jae(l0);
            cc.mov(r, c_80);
            cc.mov(x86::rcx, r);
            cc.bind(l0);
            cc.cvttss2si(r, tmp0);
            cc.add(r, x86::rcx);
            return r;
        };
        // good
        auto f32_to_int = [&](){
            x86::Gp r = cc.newInt32();
            cc.cvttss2si(r, v2r(v).as<x86::Xmm>());
            return r;
        };
        // good
        auto f32_to_f64 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.cvtss2sd(r, v2r(v).as<x86::Xmm>());
            return r;
        };
        // good
        auto uint64_to_f64 = [&](){
            x86::Xmm r = cc.newXmm();
            Label l0 = cc.newLabel();
            Label l1 = cc.newLabel();
            cc.movq(r, v2r(v).as<x86::Xmm>());
            cc.xorps(r, r);
            cc.test(x86::rcx, x86::rcx);
            cc.js(l0);
            cc.cvtsi2sd(r, x86::rcx);
            cc.jmp(l1);
            cc.bind(l0);
            cc.mov(x86::rax, x86::rcx);
            cc.and_(x86::ecx, 1);
            cc.shr(x86::rax, 1);
            cc.or_(x86::rax, x86::rcx);
            cc.cvtsi2sd(r, x86::rax);
            cc.addsd(r, r);
            cc.bind(l1);
            return r;
        };
        // good
        auto uint64_to_f32 = [&](){
            x86::Xmm r = cc.newXmm();
            Label l0 = cc.newLabel();
            Label l1 = cc.newLabel();
            cc.movq(r, v2r(v).as<x86::Xmm>());
            cc.xorps(r, r);
            cc.test(x86::rcx, x86::rcx);
            cc.js(l0);
            cc.cvtsi2ss(r, x86::rcx);
            cc.jmp(l1);
            cc.bind(l0);
            cc.mov(x86::rax, x86::rcx);
            cc.and_(x86::ecx, 1);
            cc.shr(x86::rax, 1);
            cc.or_(x86::rax, x86::rcx);
            cc.cvtsi2ss(r, x86::rax);
            cc.addss(r, r);
            cc.bind(l1);
            return r;
        };
        // good
        auto int64_to_f64 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.movq(r, v2r(v).as<x86::Xmm>());
            cc.xorps(r, r);
            cc.cvtsi2sd(r, x86::rcx);
            return r;
        };
        // good
        auto int64_to_f32 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.movq(r, v2r(v).as<x86::Xmm>());
            cc.xorps(r, r);
            cc.cvtsi2ss(r, x86::rcx);
            return r;
        };
        // good
        auto uint_to_f64 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.xorps(r, r);
            cc.cvtsi2sd(r, v2r(v).as<x86::Gp>());
            return r;
        };
        // good
        auto uint_to_f32 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.xorps(r, r);
            cc.cvtsi2ss(r, v2r(v).as<x86::Gp>());
            return r;
        };
        // good
        auto int_to_f64 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.movd(r, v2r(v).as<x86::Gp>());
            cc.cvtdq2pd(r, r);
            return r;
        };
        // good
        auto int_to_f32 = [&](){
            x86::Xmm r = cc.newXmm();
            cc.movd(r, v2r(v).as<x86::Gp>());
            cc.cvtdq2ps(r, r);
            return r;
        };
        // good
        auto int_to_int64 = [&](){
            x86::Gp r = cc.newInt64();
            cc.movsxd(r, v2r(v).as<x86::Gp>());
            return r;
        };
        auto strict_match = [&](){
            return v2r(v);
        };

        if (t1->is_primitive) {
            if (t1->is_floating_point) {
                if (t1->size == sizeof(f64)) {
                    if (t2->is_floating_point) {
                        if (t2->size == sizeof(f64)) return strict_match();
                        else return f32_to_f64();
                    } else {
                        if (t2->is_unsigned) {
                            if (t2->size == sizeof(u64)) return uint64_to_f64();
                            else return uint_to_f64();
                        } else {
                            if (t2->size == sizeof(u64)) return int64_to_f64();
                            else return int_to_f64();
                        }
                    }
                } else {
                    if (t2->is_floating_point) {
                        if (t2->size == sizeof(f64)) return f64_to_f32();
                        else return strict_match();
                    } else {
                        if (t2->is_unsigned) {
                            if (t2->size == sizeof(u64)) return uint64_to_f32();
                            else return uint_to_f32();
                        } else {
                            if (t2->size == sizeof(u64)) return int64_to_f32();
                            else return int_to_f32();
                        }
                    }
                }
            } else {
                if (t1->is_unsigned) {
                    if (t1->size == sizeof(u64)) {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) return f64_to_uint64();
                            else return f32_to_u64();
                        } else {
                            if (t2->size == sizeof(u64)) return strict_match();
                            else return int_to_int64();
                        }
                    } else {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) return f64_to_int();
                            else return f32_to_int();
                        } else {
                            // either it's an actually strict match or it narrows the type
                            // either way, this is valid
                            return strict_match();
                        }
                    }
                } else {
                    if (t1->size == sizeof(u64)) {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) return f64_to_int64();
                            else return f32_to_int();
                        } else {
                            if (t2->size == sizeof(u64)) return strict_match();
                            else return int_to_int64();
                        }
                    } else {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) return f64_to_int();
                            else return f32_to_int();
                        } else {
                            // either it's an actually strict match or it narrows the type
                            // either way, this is valid
                            return strict_match();
                        }
                    }
                }
            }
        }

        x86::Gp r = cc.newUInt64();
        cc.mov(r, v2r(v).as<x86::Gp>());
        return r;
    }

    void x86_backend::generate(compilation_output& in) {
        MyErrorHandler eh;
        StringLogger sl;
        CodeHolder ch;
        ch.init(m_rt->environment());
        ch.setErrorHandler(&eh);
        ch.setLogger(&sl);
        ch.addEmitterOptions(BaseEmitter::kValidationOptionIntermediate);
        x86::Compiler cc(&ch);
        cc.addValidationOptions(BaseEmitter::kValidationOptionIntermediate);

        func_label_map flabels;
        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            if (in.funcs[f].func->is_external) continue;
            flabels[in.funcs[f].func] = new Label(cc.newNamedLabel(in.funcs[f].func->name.c_str()));
        }

        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            if (in.funcs[f].func->is_external) continue;

            script_function* cf = in.funcs[f].func;
            function_signature* cfSig = cf->type->signature;
            cc.commentf("\n;\n; %s\n;", cfSig->to_string(cf->name, cf->is_method_of, cf->owner, false).c_str());

            FuncSignatureBuilder fsb;
            fsig(cf, fsb);
            cc.bind(*flabels[cf]);
            cc.addFunc(fsb);

            gen_function(in, f, cc, fsb, flabels);

            cc.endFunc();
        }

        if (cc.finalize()) {
            String content = std::move(sl.content());
            printf("%s\n", content.data());
            abort();
        }

        void* addr = nullptr;
        Error err = m_rt->add(&addr, &ch);
        if (err) {
            String content = std::move(sl.content());
            printf("%s\n", content.data());
            throw std::exception("JIT error");
        } else if (m_log_asm) {
            String content = std::move(sl.content());
            printf("%s\n", content.data());
        }

        for (auto& l : flabels) {
            u64 faddr = ch.baseAddress() + ch.labelOffsetFromBase(*l.getSecond());
            l.getFirst()->access.entry = faddr;
            delete l.getSecond();
        }
    }

    u16 x86_backend::gp_count() const {
        return 0;
    }

    u16 x86_backend::fp_count() const {
        return 0;
    }

    bool x86_backend::perform_register_allocation() const {
        return false;
    }

    void x86_backend::gen_function(compilation_output& in, u16 fidx, asmjit::x86::Compiler& cc, asmjit::FuncSignature& fs, func_label_map& flabels) {
        using op = compile::operation;
        using var = compile::var;
        script_function* cf = in.funcs[fidx].func;
        function_signature* cfSig = cf->type->signature;
        auto& code = in.funcs[fidx].code;
        std::vector<var> params;
        std::vector<x86::Reg> args;
        robin_hood::unordered_map<u64, x86::Reg> regs;

        u8 ectx_idx = 0;
        for (u8 a = 0;a < cfSig->args.size();a++) {
            if (cfSig->args[a].implicit == function_signature::argument::implicit_type::exec_ctx) {
                ectx_idx = a;
                break;
            }
        }

        for (u8 a = 0;a < cfSig->args.size();a++) {
            if (cfSig->args[a].tp->is_floating_point) {
                x86::Xmm arg = cc.newXmm();
                cc.setArg(a, arg);
                args.push_back(arg);
            } else {
                x86::Reg arg = cc.newReg(fs.arg(a));
                cc.setArg(a, arg);
                args.push_back(arg);
            }
        }

        auto v2r = [&](const var& v) -> x86::Reg {
            if (v.is_arg()) return args[v.arg_idx()];
            else if (v.is_imm()) {
                if (v.type()->is_floating_point) {
                    x86::Xmm r = cc.newXmm();
                    if (v.size() == sizeof(f64)) cc.movq(r, imm2const(cc, v));
                    else cc.movd(r, imm2const(cc, v));
                    return r;
                }
                x86::Gp r = cc.newGp(convertType(v.type()));
                cc.mov(r, imm2const(cc, v));
                return r;
            } else {
                auto it = regs.find(v.reg_id());
                if (it != regs.end()) return it->getSecond();
                if (v.type()->is_floating_point) return regs[v.reg_id()] = cc.newXmm();
                return regs[v.reg_id()] = cc.newReg(convertType(v.type()), v.name().c_str());
            }

            // exception
            return x86::Reg();
        };

        robin_hood::unordered_map<label_id, Label> lmap;

        for (u64 c = 0;c < code.size();c++) {
            if (code[c].op == op::label) lmap[code[c].labels[0]] = cc.newLabel();
        }

        for (u64 c = 0;c < code.size();c++) {
            const compile::tac_instruction& i = code[c];
            const var& o1 = i.operands[0];
            const var& o2 = i.operands[1];
            const var& o3 = i.operands[2];

            script_type* t1 = o1.valid() ? o1.type() : nullptr;
            u32 t1i = convertType(t1);
            script_type* t2 = o2.valid() ? o2.type() : nullptr;
            u32 t2i = convertType(t2);
            script_type* t3 = o3.valid() ? o3.type() : nullptr;
            u32 t3i = convertType(t3);

            auto setResult = [&](x86::Reg result, bool overwriteRegId = true) {
                const var* assigns = i.assignsTo();

                x86::Reg dest;
                if (assigns->is_arg()) dest = args[assigns->arg_idx()];
                else {
                    auto it = regs.find(assigns->reg_id());
                    if (it != regs.end()) dest = it->second;
                }

                if (dest.isValid()) {
                    if (assigns->type()->is_floating_point) {
                        if (result.isXmm()) {
                            cc.movq(dest.as<x86::Xmm>(), result.as<x86::Xmm>());
                        } else {
                            cc.movq(dest.as<x86::Xmm>(), result.as<x86::Gp>());
                        }
                    } else {
                        cc.mov(dest.as<x86::Gp>(), result.as<x86::Gp>());
                    }
                }

                if (overwriteRegId) {
                    if (assigns->is_arg()) args[assigns->arg_idx()] = result;
                    else regs[assigns->reg_id()] = result;
                }
            };

            // todo... use Xmm registers for 64 bit ints...
            // refactor relevant instructions to account for this
            cc.commentf("\n; [%d] %s", c, i.to_string().c_str());
            switch(i.op) {
                case op::null: {
                    break;
                }
                case op::term: {
                    break;
                }
                case op::load: {
                    // load dest_var imm_addr
                    // load dest_var var_addr
                    // load dest_var var_addr imm_offset
                    x86::Mem src;
                    if (o2.is_imm()) src = x86::ptr(o2.imm_u(), o1.size());
                    else if (o2.is_arg()) src = cc.ptr_base(args[o2.arg_idx()].as<x86::Gp>().id(), o3.is_imm() ? (i32)o3.imm_u() : 0, o1.size());
                    else src = cc.ptr_base(regs[o2.reg_id()].as<x86::Gp>().id(), o3.is_imm() ? (i32)o3.imm_u() : 0, o1.size());

                    if (t1->is_floating_point){
                        if (t1->size == sizeof(f64)) cc.movsd(v2r(o1).as<x86::Xmm>(), src);
                        else cc.movss(v2r(o1).as<x86::Xmm>(), src);
                    } else cc.mov(v2r(o1).as<x86::Gp>(), src);

                    break;
                }
                case op::store: {
                    x86::Mem dst;
                    if (o1.is_arg()) dst = cc.ptr_base(args[o1.arg_idx()].as<x86::Gp>().id(), 0, o2.size());
                    else if (o1.is_reg()) dst = cc.ptr_base(regs[o1.reg_id()].as<x86::Gp>().id(), 0, o2.size());

                    if (o2.is_imm()) {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) cc.movsd(dst, v2r(o2).as<x86::Xmm>());
                            else cc.movss(dst, v2r(o2).as<x86::Xmm>());
                        } 
                        else {
                            if (o2.imm_u() >= UINT16_MAX) cc.mov(dst, v2r(o2).as<x86::Gp>());
                            else cc.mov(dst, v2imm(o2));
                        }
                    }
                    else {
                        if (t2->is_floating_point) {
                            if (t2->size == sizeof(f64)) cc.movsd(dst, v2r(o2).as<x86::Xmm>());
                            else cc.movss(dst, v2r(o2).as<x86::Xmm>());
                        }
                        else cc.mov(dst, v2r(o2).as<x86::Gp>());
                    }
                    break;
                }
                case op::stack_alloc: {
                    x86::Mem m = cc.newStack((u32)o2.imm_u(), 4);
                    x86::Gp r = cc.newIntPtr();
                    cc.lea(r, m);
                    setResult(r);
                    break;
                }
                case op::stack_free: {
                    break;
                }
                case op::module_data: {
                    script_module* mod = o2.imm_u() == in.mod->id() ? in.mod : m_ctx->module((u32)o2.imm_u());
                    if (mod) {
                        auto m = METHOD_PTR(script_buffer, data, void*, u64);
                        auto md = &ffi::call_class_method<void*, script_buffer, void* (script_buffer::*)(u64), script_buffer*, u64>;
                        u64 mdata = *(u64*)reinterpret_cast<void*>(&md);

                        InvokeNode* call;
                        cc.invoke(&call, mdata, FuncSignatureT<u64, u64, void*, void*, u64>());
                        call->setArg(0, *(u64*)reinterpret_cast<void*>(&m));
                        call->setArg(1, args[ectx_idx]);
                        call->setArg(2, (void*)mod->data());
                        call->setArg(3, o3.imm_u());
                        x86::Reg r = cc.newReg(Type::kIdU64);
                        call->setRet(0, r);
                        setResult(r);
                    } else {
                        // exception
                    }
                    break;
                }
                case op::reserve: break;
                case op::uadd: [[fallthrough]];
                case op::iadd: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    if (o3.is_imm()) cc.add(r, v2imm(o3));
                    else cc.add(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::usub: [[fallthrough]];
                case op::isub: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    if (o3.is_imm()) cc.sub(r, v2imm(o3));
                    else cc.sub(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::umul: [[fallthrough]];
                case op::imul: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());
                    
                    if (o3.is_imm()) cc.imul(r, v2imm(o3));
                    else cc.imul(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::idiv: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    x86::Gp rem = cc.newInt32("rem");
                    cc.xor_(rem, rem);
                    if (o3.is_imm()) cc.idiv(rem, r, imm2const(cc, o3));
                    else cc.idiv(rem, r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::imod: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    x86::Gp rem = cc.newInt32("rem");
                    cc.xor_(rem, rem);
                    if (o3.is_imm()) cc.idiv(rem, r, imm2const(cc, o3));
                    else cc.idiv(rem, r, v2r(o3).as<x86::Gp>());

                    setResult(rem);
                    break;
                }
                case op::udiv: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    x86::Gp rem = cc.newInt32("rem");
                    cc.xor_(rem, rem);
                    if (o3.is_imm()) cc.div(rem, r, imm2const(cc, o3));
                    else cc.div(rem, r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::umod: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    x86::Gp rem = cc.newInt32("rem");
                    cc.xor_(rem, rem);
                    if (o3.is_imm()) cc.div(rem, r, imm2const(cc, o3));
                    else cc.div(rem, r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::fadd: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movss(r, imm2const(cc, o2));
                    else cc.movss(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.addss(r, imm2const(cc, o3));
                    else cc.addss(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::fsub: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movss(r, imm2const(cc, o2));
                    else cc.movss(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.subss(r, imm2const(cc, o3));
                    else cc.subss(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::fmul: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movss(r, imm2const(cc, o2));
                    else cc.movss(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.mulss(r, imm2const(cc, o3));
                    else cc.mulss(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::fdiv: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movss(r, imm2const(cc, o2));
                    else cc.movss(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.divss(r, imm2const(cc, o3));
                    else cc.divss(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::fmod: {
                    InvokeNode* call = nullptr;
                    cc.invoke(&call, fmodf, FuncSignatureT<f32, f32, f32>());
                    if (o2.is_imm()) call->setArg(0, v2imm(o2));
                    else call->setArg(0, v2r(o2));
                    if (o3.is_imm()) call->setArg(1, v2imm(o3));
                    else call->setArg(1, v2r(o3));

                    x86::Xmm r = v2r(o1).as<x86::Xmm>();
                    call->setRet(0, r);
                    setResult(r);
                    break;
                }
                case op::dadd: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movsd(r, imm2const(cc, o2));
                    else cc.movsd(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.addsd(r, imm2const(cc, o3));
                    else cc.addsd(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::dsub: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movsd(r, imm2const(cc, o2));
                    else cc.movsd(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.subsd(r, imm2const(cc, o3));
                    else cc.subsd(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::dmul: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movsd(r, imm2const(cc, o2));
                    else cc.movsd(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.mulsd(r, imm2const(cc, o3));
                    else cc.mulsd(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::ddiv: {
                    x86::Xmm r = cc.newXmm();

                    if (o2.is_imm()) cc.movsd(r, imm2const(cc, o2));
                    else cc.movsd(r, v2r(o2).as<x86::Xmm>());

                    if (o3.is_imm()) cc.divsd(r, imm2const(cc, o3));
                    else cc.divsd(r, v2r(o3).as<x86::Xmm>());

                    setResult(r);
                    break;
                }
                case op::dmod: {
                    InvokeNode* call = nullptr;
                    cc.invoke(&call, FUNC_PTR(fmod, f64, f64, f64), FuncSignatureT<f64, f64, f64>());
                    if (o2.is_imm()) call->setArg(0, v2imm(o2));
                    else call->setArg(0, v2r(o2));
                    if (o3.is_imm()) call->setArg(1, v2imm(o3));
                    else call->setArg(1, v2r(o3));

                    x86::Xmm r = v2r(o1).as<x86::Xmm>();
                    call->setRet(0, r);
                    setResult(r);
                    break;
                }
                case op::sl: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    if (o3.is_imm()) cc.shl(r, v2imm(o3));
                    else cc.shl(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::sr: {
                    x86::Gp r = cc.newGp(t1i);

                    if (o2.is_imm()) cc.mov(r, v2imm(o2));
                    else cc.mov(r, v2r(o2).as<x86::Gp>());

                    if (o3.is_imm()) cc.shr(r, v2imm(o3));
                    else cc.shr(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::land: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label f = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), 0);
                    cc.je(f);
                    cc.cmp(v2r(o3).as<x86::Gp>(), 0);
                    cc.je(f);
                    cc.mov(r, 1);
                    cc.jmp(t);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(t);

                    setResult(r);
                    break;
                }
                case op::lor: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label f = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), 0);
                    cc.jne(f);
                    cc.cmp(v2r(o3).as<x86::Gp>(), 0);
                    cc.jne(f);
                    cc.mov(r, 0);
                    cc.jmp(t);
                    cc.bind(f);
                    cc.mov(r, 1);
                    cc.bind(t);

                    setResult(r);
                    break;
                }
                case op::band: {
                    x86::Gp r = cc.newGp(t2i);

                    cc.mov(r, v2r(o2).as<x86::Gp>());
                    cc.and_(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::bor: {
                    x86::Gp r = cc.newGp(t2i);

                    cc.mov(r, v2r(o2).as<x86::Gp>());
                    cc.or_(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::bxor: {
                    x86::Gp r = cc.newGp(t2i);

                    cc.mov(r, v2r(o2).as<x86::Gp>());
                    cc.xor_(r, v2r(o3).as<x86::Gp>());

                    setResult(r);
                    break;
                }
                case op::ult: [[fallthrough]];
                case op::ilt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.jl(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::ugt: [[fallthrough]];
                case op::igt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.jg(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::ulte: [[fallthrough]];
                case op::ilte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.jle(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::ugte: [[fallthrough]];
                case op::igte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.jge(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::uncmp: [[fallthrough]];
                case op::incmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.jne(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::ucmp: [[fallthrough]];
                case op::icmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label t = cc.newLabel();
                    Label e = cc.newLabel();
                    cc.cmp(v2r(o2).as<x86::Gp>(), v2r(o3).as<x86::Gp>());
                    cc.je(t);
                    cc.mov(r, 0);
                    cc.jmp(e);
                    cc.bind(t);
                    cc.mov(r, 1);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::flt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comiss(v2r(o3).as<x86::Xmm>(), v2r(o2).as<x86::Xmm>());
                    cc.jbe(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::fgt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comiss(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jbe(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::flte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comiss(v2r(o3).as<x86::Xmm>(), v2r(o2).as<x86::Xmm>());
                    cc.jb(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::fgte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comiss(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jb(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::fncmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.ucomiss(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jp(f);
                    cc.je(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::fcmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.ucomiss(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jp(f);
                    cc.jne(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dlt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comisd(v2r(o3).as<x86::Xmm>(), v2r(o2).as<x86::Xmm>());
                    cc.jbe(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dgt: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comisd(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jbe(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dlte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comisd(v2r(o3).as<x86::Xmm>(), v2r(o2).as<x86::Xmm>());
                    cc.jb(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dgte: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.comisd(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jb(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dncmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.ucomisd(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jp(f);
                    cc.je(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::dcmp: {
                    x86::Gp r = cc.newGp(Type::IdOfT<bool>::kTypeId);

                    Label f = cc.newLabel();
                    Label e = cc.newLabel();

                    cc.ucomisd(v2r(o2).as<x86::Xmm>(), v2r(o3).as<x86::Xmm>());
                    cc.jp(f);
                    cc.jne(f);
                    cc.mov(r, 1);
                    cc.jmp(e);
                    cc.bind(f);
                    cc.mov(r, 0);
                    cc.bind(e);

                    setResult(r);
                    break;
                }
                case op::resolve:
                case op::eq: {
                    setResult(convert(t1, t2, o2, cc, args, regs));
                    break;
                }
                case op::neg: {
                    if (t1->is_floating_point) {
                        if (t1->size == sizeof(f64)) {
                            u64 a[2] = { 8000000000000000, 0x8000000000000000 };
                            auto c_80 = cc.newConst(ConstPool::kScopeGlobal, &a, 16);
                            x86::Xmm r = cc.newXmm();
                            cc.movsd(r, v2r(o2).as<x86::Xmm>());
                            cc.xorps(r, c_80);
                            setResult(r);
                        } else {
                            u64 a[2] = { 0x8000000080000000, 0x8000000080000000 };
                            auto c_4016 = cc.newConst(ConstPool::kScopeGlobal, a, 16);
                            x86::Xmm r = cc.newXmm();
                            cc.movss(r, v2r(o2).as<x86::Xmm>());
                            cc.xorps(r, c_4016);
                            setResult(r);
                        }
                    } else {
                        x86::Gp r = cc.newGp(t1i);
                        cc.mov(r, v2r(o2).as<x86::Gp>());
                        cc.neg(r);
                        setResult(r);
                    }
                    break;
                }
                case op::call: {
                    function_signature* sig = nullptr;
                    if (i.callee) sig = i.callee->type->signature;
                    else sig = i.callee_v.type()->signature;

                    FuncSignatureBuilder cs;
                    void* addr = nullptr;
                    Label* lbl = nullptr;
                    x86::Reg retAddr;
                    x86::Reg retReg;
                    std::vector<Imm> implicitArgs;
                    std::vector<Operand> fargs;
                    std::vector<script_type*> fargts;
                    InvokeNode* call = nullptr;

                    if (sig->return_type->size > 0 && !sig->returns_on_stack) {
                        if (sig->returns_pointer) {
                            retReg = cc.newUIntPtr();
                        } else if (sig->return_type->is_primitive) {
                            if (sig->return_type->is_floating_point) {
                                retReg = cc.newXmm();
                            } else {
                                retReg = cc.newReg(convertType(sig->return_type));
                            }
                        }
                    }

                    auto passArg = [&](u8 aidx, const var& p) {
                        auto& a = sig->args[aidx];
                        fargts.push_back(p.type());
                        if (p.is_imm() && !a.is_ptr && !p.type()->is_floating_point) fargs.push_back(v2imm(p));
                        else {
                            if (p.type()->is_primitive && a.is_ptr) {
                                // objects are passed as pointers no matter what, but primitives
                                // are always by value unless specified by host function signature

                                x86::Mem dst = cc.newStack((u32)p.size(), 4);

                                if (p.type()->is_floating_point) cc.movd(dst, v2r(p).as<x86::Xmm>());
                                else cc.mov(dst, v2r(p).as<x86::Gp>());

                                x86::Gp r = cc.newUInt64();
                                cc.lea(r, dst);
                                fargs.push_back(r);
                            } else {
                                fargs.push_back(v2r(p));//convert(a.tp, p.type(), p, cc, args, regs));
                            }
                        }
                    };
                    auto finalizeCall = [&]() {
                        call->setUserDataAsUInt64(c);
                        u8 a = 0;
                        if (retAddr.isValid()) call->setArg(a++, retAddr);
                        for (auto& i : implicitArgs) call->setArg(a++, i);
                        for (u8 g = 0;g < fargs.size();g++) {
                            u32 at = cs.arg(a);
                            u32 pt = convertType(fargts[g]);
                            if (fargs[g].isImm()) call->setArg(a++, fargs[g].as<Imm>());
                            else if (fargts[g]->is_floating_point) call->setArg(a++, fargs[g].as<x86::Xmm>());
                            else call->setArg(a++, fargs[g].as<x86::Gp>());
                        }

                        if (sig->return_type->size > 0 && !sig->returns_on_stack) {
                            if (sig->returns_pointer) {
                                call->setRet(0, retReg);
                                setResult(retReg, false);
                            } else if (sig->return_type->is_primitive) {
                                if (sig->return_type->is_floating_point) {
                                    call->setRet(0, retReg);
                                    setResult(retReg, false);
                                } else {
                                    call->setRet(0, retReg);
                                    setResult(retReg, false);
                                }
                            }
                        }
                    };

                    if (i.callee) {
                        if (i.callee->is_host) {
                            if (i.callee->is_external && !i.callee->access.wrapped) {
                                // unresolved external (cdecl)
                                
                                fsig(i.callee, cs);

                                for (u8 a = 0;a < params.size();a++) {
                                    passArg(a, params[a]);
                                }
                            }
                            else if (i.callee->access.wrapped->srv_wrapper_func) {
                                if (i.callee->access.wrapped->call_method_func) {
                                    // ret ptr, call_method_func, func_ptr, call_params...
                                    fsig(i.callee, cs);
                                    addr = i.callee->access.wrapped->srv_wrapper_func;

                                    u8 raidx = 0;
                                    for (;raidx < sig->args.size();raidx++) {
                                        if (sig->args[raidx].implicit == function_signature::argument::implicit_type::ret_addr) {
                                            retAddr = v2r(params[raidx]);
                                            params.erase(params.begin() + raidx);
                                            break;
                                        }
                                    }

                                    implicitArgs.push_back(i.callee->access.wrapped->call_method_func);
                                    implicitArgs.push_back(i.callee->access.wrapped->func_ptr);

                                    u8 a = 0;
                                    for (var& p : params) {
                                        passArg(a >= raidx ? a + 1 : a, p);
                                        a++;
                                    }
                                } else {
                                    // ret ptr, func_ptr, call_params...
                                    fsig(i.callee, cs);
                                    addr = i.callee->access.wrapped->srv_wrapper_func;

                                    u8 raidx = 0;
                                    for (;raidx < sig->args.size();raidx++) {
                                        if (sig->args[raidx].implicit == function_signature::argument::implicit_type::ret_addr) {
                                            retAddr = v2r(params[raidx]);
                                            params.erase(params.begin() + raidx);
                                            break;
                                        }
                                    }

                                    implicitArgs.push_back(i.callee->access.wrapped->func_ptr);

                                    u8 a = 0;
                                    for (var& p : params) {
                                        passArg(a >= raidx ? a + 1 : a, p);
                                        a++;
                                    }
                                }
                            } else if (i.callee->access.wrapped->call_method_func) {
                                // func_ptr, call_params
                                fsig(i.callee, cs);
                                addr = i.callee->access.wrapped->call_method_func;
                                implicitArgs.push_back(i.callee->access.wrapped->func_ptr);

                                for (u8 a = 0;a < params.size();a++) {
                                    passArg(a, params[a]);
                                }
                            } else {
                                // func_ptr, params...
                                fsig(i.callee, cs);
                                addr = i.callee->access.wrapped->cdecl_wrapper_func;
                                implicitArgs.push_back(i.callee->access.wrapped->func_ptr);

                                for (u8 a = 0;a < params.size();a++) {
                                    passArg(a, params[a]);
                                }
                            }
                        } else {
                            fsig(i.callee, cs);
                            if (i.callee->access.entry) addr = (void*)i.callee->access.entry;
                            else lbl = flabels[i.callee];

                            for (u8 a = 0;a < params.size();a++) {
                                passArg(a, params[a]);
                            }
                        }

                        if (i.callee->is_external && !i.callee->access.wrapped) {
                            // must determine function pointer at runtime
                            x86::Gp wrappedPtr = cc.newIntPtr();
                            x86::Mem src = cc.intptr_ptr(reinterpret_cast<u64>((void*)&i.callee->access.wrapped));
                            cc.mov(wrappedPtr, src);
                            cc.mov(wrappedPtr, cc.ptr_base(wrappedPtr.id(), offsetof(ffi::cdecl_pointer, func_ptr), sizeof(void*)));
                            cc.invoke(&call, wrappedPtr, cs);
                        } else {
                            if (addr) cc.invoke(&call, addr, cs);
                            else cc.invoke(&call, *lbl, cs);
                        }

                        finalizeCall();
                    } else {
                        auto buildCall = [&](const x86::Gp& fptr, bool is_host, bool is_srv, bool is_nonstatic_method) {
                            cs.init(CallConv::kIdCDecl, FuncSignature::kNoVarArgs, Type::kIdVoid, cs._builderArgList, 0);
                            fsig(sig, cs, is_host, is_srv, is_nonstatic_method);
                            cc.invoke(&call, fptr, cs);

                            if (is_host) {
                                if (is_srv) {
                                    u8 idx = 0;
                                    for (u8 a = 0;a < sig->args.size();a++) {
                                        if (sig->args[a].implicit == function_signature::argument::implicit_type::ret_addr) {
                                            // handled separately
                                            retAddr = v2r(params[a]);
                                            continue;
                                        }
                                        
                                        if (sig->args[a].implicit == function_signature::argument::implicit_type::capture_data_ptr) {
                                            // host callbacks accept the capture data argument, but never store capture data
                                            fargs.push_back(Imm(0));
                                            fargts.push_back(type_of<void*>(m_ctx));
                                            continue;
                                        }

                                        passArg(a, params[idx++]);
                                    }
                                } else {
                                    u8 idx = 0;
                                    for (u8 a = 0;a < sig->args.size();a++) {
                                        if (sig->args[a].implicit == function_signature::argument::implicit_type::capture_data_ptr) {
                                            // host callbacks accept the capture data argument, but never store capture data
                                            fargs.push_back(Imm(0));
                                            fargts.push_back(type_of<void*>(m_ctx));
                                            continue;
                                        }

                                        passArg(a, params[idx++]);
                                    }
                                }
                            } else {
                                u8 idx = 0;
                                for (u8 a = 0;a < sig->args.size();a++) {
                                    if (sig->args[a].implicit == function_signature::argument::implicit_type::capture_data_ptr) {
                                        // handled separately
                                        continue;
                                    }

                                    passArg(a, params[idx++]);
                                }
                            }
                            finalizeCall();

                            retAddr.reset();
                            cs.reset();
                            implicitArgs.clear();
                            fargs.clear();
                            fargts.clear();
                        };

                        const var& cb = i.callee_v;
                        // cb actually refers to a raw_callback**
                        x86::Gp fpPtr = cc.newUIntPtr();
                        x86::Gp sfPtr = cc.newUIntPtr();
                        x86::Gp isHostV = cc.newGp(Type::IdOfT<bool>::kTypeId);
                        Label isHostLabel = cc.newLabel();
                        Label endLabel = cc.newLabel();

                        /*
                         * 1  | raw_callback* a = *cb;
                         * 2  | function_pointer* b = a->ptr;
                         * 3  | script_function* f = b->target;
                         * 4  | void* rawFuncPtr, srvFuncPtr, callMethodPtr, firstArgPtr;
                         * 5  | rawFuncPtr = (void*)f->access.entry;
                         * 6  | bool isHostV = f->is_host;
                         * 7  | if (!isHostV) {
                         * 8  |     firstArgPtr = b->data;
                         * 9  |     call rawFuncPtr with params = [firstArgPtr, params...]
                         * 10 |     goto end;
                         * 11 | } else {
                         * 12 |     firstArgPtr = b->m_this;
                         * 13 |     [if sig->returns_on_stack] srvFuncPtr = (void*)f->access.wrapped->srv_wrapper_func;
                         * 14 |     callMethodPtr = (void*)f->access.wrapped->call_method_func;
                         * 15 |     rawFuncPtr = (void*)f->access.wrapped->func_ptr;
                         * 16 |     [if sig->returns_on_stack] call srvFuncPtr with params = [callMethodPtr, rawFuncPtr, firstArgPtr, params...]
                         * 17 |     [else] call callMethodPtr with params = [rawFuncPtr, firstArgPtr, params...]
                         * 18 | }
                         * 19 | end:
                         */
                        
                        // 1  | raw_callback* a = *cb;
                        cc.mov(fpPtr, cc.intptr_ptr(v2r(cb).as<x86::Gp>(), 0));

                        // 2  | function_pointer* b = a->ptr;
                        cc.mov(fpPtr, cc.intptr_ptr(fpPtr, offsetof(raw_callback, ptr)));

                        // 3  | script_function* f = b->target;
                        cc.mov(sfPtr, cc.intptr_ptr(fpPtr, offsetof(function_pointer, target)));
                        // 4  | void* rawFuncPtr, srvFuncPtr, callMethodPtr, firstArgPtr;
                        x86::Gp rawFuncPtr = cc.newUIntPtr();
                        x86::Gp srvFuncPtr = cc.newUIntPtr();
                        x86::Gp callMethodPtr = cc.newUIntPtr();
                        x86::Gp firstArgPtr = cc.newUIntPtr();
                        // 5  | rawFuncPtr = (void*)f->access;
                        cc.mov(rawFuncPtr, cc.intptr_ptr(sfPtr, offsetof(script_function, access)));

                        // 6  | bool isHostV = f->is_host;
                        cc.mov(isHostV, cc.ptr_base(sfPtr.id(), offsetof(script_function, is_host), 1));

                        // 7  | if (!isHostV) {
                        cc.cmp(isHostV, 1);
                        cc.je(isHostLabel);
                        
                        // 8  |     firstArgPtr = b->data;
                        cc.mov(firstArgPtr, cc.intptr_ptr(fpPtr, offsetof(function_pointer, data)));

                        // 9  |     call rawFuncPtr with params = [firstArgPtr, params...]
                        fargs.push_back(firstArgPtr);
                        fargts.push_back(type_of<void*>(m_ctx));
                        buildCall(rawFuncPtr, false, false, false);

                        // 10 |     goto end;
                        cc.jmp(endLabel);

                        // 11 | } else {
                        cc.bind(isHostLabel);

                        // 12 |     firstArgPtr = b->m_this;
                        cc.mov(firstArgPtr, cc.intptr_ptr(fpPtr, function_pointer::offset_of_self()));

                        // 13 |     [if sig->returns_on_stack] srvFuncPtr = (void*)f->access.wrapped->srv_wrapper_func;
                        if (sig->returns_on_stack) {
                            cc.mov(srvFuncPtr, cc.intptr_ptr(rawFuncPtr, offsetof(ffi::wrapped_function, srv_wrapper_func)));
                        }

                        // 14 |     callMethodPtr = (void*)f->access.wrapped->call_method_func;
                        cc.mov(callMethodPtr, cc.intptr_ptr(rawFuncPtr, offsetof(ffi::wrapped_function, call_method_func)));
                        
                        // 15 |     rawFuncPtr = (void*)f->access.wrapped->func_ptr;
                        cc.mov(rawFuncPtr, cc.intptr_ptr(rawFuncPtr, offsetof(ffi::wrapped_function, func_ptr)));

                        // 16 |     [if sig->returns_on_stack] call srvFuncPtr with params = [callMethodPtr, rawFuncPtr, firstArgPtr, params...]
                        if (sig->returns_on_stack) {
                            
                            fargs.push_back(callMethodPtr);
                            fargs.push_back(rawFuncPtr);
                            fargs.push_back(firstArgPtr);
                            fargts.push_back(type_of<void*>(m_ctx));
                            fargts.push_back(type_of<void*>(m_ctx));
                            fargts.push_back(type_of<void*>(m_ctx));
                            buildCall(srvFuncPtr, true, true, true);
                        }

                        // 17 |     [else] call callMethodPtr with params = [rawFuncPtr, firstArgPtr, params...]
                        else {
                            fargs.push_back(rawFuncPtr);
                            fargs.push_back(firstArgPtr);
                            fargts.push_back(type_of<void*>(m_ctx));
                            fargts.push_back(type_of<void*>(m_ctx));
                            buildCall(callMethodPtr, true, false, true);
                        }
                        // 18 | }
                        
                        // 19 | end:
                        cc.bind(endLabel);
                    }

                    if (retReg.isValid()) {
                        if (sig->return_type->is_primitive && !sig->return_type->is_floating_point && sig->return_type->size <= sizeof(u16)) {
                            // see comment in case op::ret
                            x86::Gp r = cc.newGp(convertType(sig->return_type));
                            cc.mov(r, retReg.as<x86::Gp>());
                            retReg = r;
                        }
                        
                        const var* assigns = i.assignsTo();
                        if (assigns->is_arg()) args[assigns->arg_idx()] = retReg;
                        else regs[assigns->reg_id()] = retReg;
                    }

                    params.clear();
                    break;
                }
                case op::param: {
                    params.push_back(o1);
                    break;
                }
                case op::ret: {
                    if (cfSig->return_type->size == 0 || cfSig->returns_on_stack) cc.ret();
                    else {
                        if (o1.is_reg()) {
                            if (t1->is_primitive && !t1->is_floating_point && t1->size <= sizeof(u16)) {
                                // Don't fuckin ask me why this is necessary
                                // asmjit errors without it and it took five
                                // of my post-work hours to figure this out.

                                // Like what? regs[o1.reg_id()] is ALREADY a
                                // Gp register. Why does it need to be moved
                                // to another Gp register for cc.ret(...) to
                                // not fail validation? Whatever. Some day I
                                // will figure out x86 assembly.
                                x86::Gp r = cc.newGp(t1i);
                                cc.mov(r, regs[o1.reg_id()].as<x86::Gp>());
                                cc.ret(r);
                            }
                            else cc.ret(regs[o1.reg_id()]);
                        }
                        else if (o1.is_arg()) cc.ret(args[o1.arg_idx()]);
                        else if (o1.is_imm()) {
                            Imm v = v2imm(o1);
                            x86::Reg r;
                            if (t1->is_floating_point) {
                                r = cc.newXmm();
                                cc.movd(r.as<x86::Xmm>(), imm2const(cc, o1));
                            } else {
                                r = cc.newReg(t1i);
                                cc.mov(r.as<x86::Gp>(), v);
                            }
                            cc.ret(r);
                        } else {
                            // returning void from a non-void function (exception bubbling up)
                            cc.ret();
                        }
                    }
                    break;
                }
                case op::cvt: {
                    // covered by eq
                    break;
                }
                case op::label: {
                    cc.bind(lmap[i.labels[0]]);
                    break;
                }
                case op::branch: {
                    x86::Gp cond = cc.newGp(Type::IdOfT<bool>::kTypeId);
                    if (o1.type()->is_floating_point) {
                    } else {
                        cc.cmp(v2r(o1).as<x86::Gp>(), 0);
                        cc.je(lmap[i.labels[0]]);
                    }
                    break;
                }
                case op::jump: {
                    if (i.labels[0]) cc.jmp(lmap[i.labels[0]]);
                    else cc.jmp(v2r(o1).as<x86::Gp>());
                    break;
                }
            }
        }
    }

    void x86_backend::call(script_function* func, void* ret, void** args) {
        function_signature* sig = func->type->signature;

        DCCallVM* cvm = m_ctx->call_vm();
        if (func->is_host) {
            func->access.wrapped->call(cvm, ret, args);
            return;
        }

        dcMode(cvm, DC_CALL_C_DEFAULT);
        dcReset(cvm);

        i8 argOff = 0;
        for (u8 a = 0;a < sig->args.size();a++) {
            if (sig->args[a].implicit == function_signature::argument::implicit_type::ret_addr) {
                dcArgPointer(cvm, ret);
                argOff--;
                continue;
            }

            script_type* tp = sig->args[a].tp;
            if (tp->is_primitive) {
                if (tp->is_floating_point) {
                    if (tp->size == sizeof(f64)) dcArgDouble(cvm, *(f64*)&args[a + argOff]);
                    else dcArgFloat(cvm, *(f32*)&args[a + argOff]);
                } else {
                    switch (tp->size) {
                        case sizeof(i8): {
                            if (tp->name == "bool") dcArgBool(cvm, *(bool*)&args[a + argOff]);
                            else dcArgChar(cvm, *(i8*)&args[a + argOff]);
                            break;
                        }
                        case sizeof(i16): { dcArgShort(cvm, *(i16*)&args[a + argOff]); break; }
                        case sizeof(i32): {
                            if (tp->is_unsigned) dcArgLong(cvm, *(u32*)&args[a + argOff]);
                            else dcArgInt(cvm, *(i32*)&args[a + argOff]);
                            break;
                        }
                        case sizeof(i64): { dcArgLongLong(cvm, *(i64*)&args[a + argOff]); break; }
                    }
                }
            } else {
                dcArgPointer(cvm, args[a + argOff]);
            }
        }

        script_type* rtp = sig->return_type;
        void* f = reinterpret_cast<void*>(func->access.entry);

        if (rtp->size == 0) dcCallVoid(cvm, f);
        else {
            if (rtp->is_primitive) {
                if (rtp->is_floating_point) {
                    if (rtp->size == sizeof(f64)) *(f64*)ret = dcCallDouble(cvm, f);
                    else *(f32*)ret = dcCallFloat(cvm, f);
                } else {
                    switch (rtp->size) {
                        case sizeof(i8): {
                            if (rtp->name == "bool") *(bool*)ret = dcCallBool(cvm, f);
                            else *(i8*)ret = dcCallChar(cvm, f);
                            break;
                        }
                        case sizeof(i16): { *(i16*)ret = dcCallShort(cvm, f); break; }
                        case sizeof(i32): { *(i32*)ret = dcCallInt(cvm, f); break; }
                        case sizeof(i64): { *(i64*)ret = dcCallLongLong(cvm, f); break; }
                    }
                }
            } else {
                dcCallVoid(cvm, f);
            }
        }
    }
};
