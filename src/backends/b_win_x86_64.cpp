#include <backends/b_win_x86_64.h>
#include <common/script_module.h>
#include <common/script_function.h>
#include <common/script_type.h>
#include <common/script_context.h>
#include <builtin/script_buffer.h>
#include <util/util.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace orc;

namespace gjs {
    Type* llvm_type(script_type* tp, LLVMContext* ctx, robin_hood::unordered_map<u64, llvm::Type*>& types) {
        if (tp->is_primitive) {
            if (tp->is_floating_point) {
                if (tp->size == sizeof(f64)) return Type::getDoubleTy(*ctx);
                else return Type::getFloatTy(*ctx);
            } else {
                switch (tp->size) {
                    case 0: return Type::getVoidTy(*ctx);
                    case 1: {
                        if (tp->name == "bool") return Type::getInt1Ty(*ctx);
                        return Type::getInt8Ty(*ctx);
                    }
                    case 2: return Type::getInt16Ty(*ctx);
                    case 4: return Type::getInt32Ty(*ctx);
                    case 8: return Type::getInt64Ty(*ctx);
                }
            }
        } else {
            return Type::getInt64Ty(*ctx); // structs are always pointers in gjs ir

            u64 id = join_u32(tp->owner->id(), tp->id());
            auto it = types.find(id);
            if (it == types.end()) return nullptr;
            return it->second;
        }

        return nullptr;
    }

    Value* llvm_value(const compile::var& v, robin_hood::unordered_map<u64, Value*>& vars, Function* f, LLVMContext* ctx, robin_hood::unordered_map<u64, llvm::Type*>& types) {
        if (v.is_imm()) {
            if (v.type()->is_floating_point) {
                if (v.type()->size == sizeof(f64)) {
                    return ConstantFP::get(llvm_type(v.type(), ctx, types), (double)v.imm_d());
                } else {
                    return ConstantFP::get(llvm_type(v.type(), ctx, types), (float)v.imm_f());
                }
            } else {
                return ConstantInt::get(llvm_type(v.type(), ctx, types), v.imm_u(), !v.type()->is_unsigned);
            }
        } else if (v.is_arg()) {
            return f->getArg(v.arg_idx());
        } else {
            auto it = vars.find(v.reg_id());
            if (it == vars.end()) return nullptr;
            return it->second;
        }

        return nullptr;
    }

    win64_backend::win64_backend(int argc, char* argv[]) : m_log_ir(false) {
        // Initialize LLVM.
        m_llvm_init = new InitLLVM(argc, argv);

        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetAsmParser();

        m_llvm = std::make_unique<LLVMContext>();
        auto jit = LLJITBuilder().create();
        if (!jit) {
            // exception
        }

        m_jit = std::move(jit.get());
    }

    win64_backend::~win64_backend() {
        delete m_llvm_init;
    }

    void win64_backend::init() {
        auto& DL = m_jit->getDataLayout();
        auto& ES = m_jit->getExecutionSession();
        auto& lib = m_jit->getMainJITDylib();

        auto mdata = &bind::call_class_method<void*, script_buffer, void* (script_buffer::*)(u64), script_buffer*, u64>;
        SymbolMap symbols;
        symbols[ES.intern("__mdata")] = { pointerToJITTargetAddress(mdata), JITSymbolFlags::Exported | JITSymbolFlags::Absolute };

        auto global_funcs = m_ctx->global()->functions();
        for (u32 f = 0;f < global_funcs.size();f++) {
            void* fptr = global_funcs[f]->access.wrapped->func_ptr;
            
            if (global_funcs[f]->access.wrapped->srv_wrapper_func) fptr = global_funcs[f]->access.wrapped->srv_wrapper_func;
            else if (global_funcs[f]->access.wrapped->call_method_func) fptr = global_funcs[f]->access.wrapped->call_method_func;

            symbols[ES.intern(internal_func_name(global_funcs[f]))] = {
                pointerToJITTargetAddress(fptr),
                JITSymbolFlags::Exported | JITSymbolFlags::Absolute
            };
        }

        lib.define(absoluteSymbols(symbols));
    }

    void win64_backend::generate(compilation_output& in) {
        std::unique_ptr<Module> mod = std::make_unique<Module>(in.mod->name(), *m_llvm.getContext());

        add_bindings_to_module(mod.get());

        for (u16 t = 0;t < in.types.size();t++) {
            std::vector<Type*> fields;
            for (u16 f = 0;f < in.types[t]->properties.size();f++) {
                fields.push_back(llvm_type(in.types[t]->properties[f].type, m_llvm.getContext(), m_types));
            }

            Type* tp = StructType::create(*m_llvm.getContext(), fields, in.types[t]->name, true);
            m_types[join_u32(in.mod->id(), in.types[t]->id())] = tp;
        }

        // forward declare
        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;

            std::vector<Type*> ptypes;
            for (u8 a = 0;a < in.funcs[f].func->signature.arg_types.size();a++) {
                ptypes.push_back(llvm_type(in.funcs[f].func->signature.arg_types[a], m_llvm.getContext(), m_types));
            }

            Function* func = Function::Create(
                FunctionType::get(llvm_type(in.funcs[f].func->signature.return_type, m_llvm.getContext(), m_types), ptypes, false),
                Function::ExternalLinkage,
                internal_func_name(in.funcs[f].func),
                mod.get()
            );
        }

        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            generate_function(in, in.funcs[f], mod.get());
        }

        if (m_log_ir) {
            std::string s;
            auto os = raw_string_ostream(s);
            mod->print(os, nullptr);
            printf(
                "-------- LLVM IR --------\n"
                "%s"
                "-------------------------\n", 
            s.c_str());
        }

        m_jit->addIRModule(ThreadSafeModule(std::move(mod), m_llvm));

        for (u16 f = 0;f < in.funcs.size();f++) {
            in.funcs[f].func->access.entry = m_jit->lookup(internal_func_name(in.funcs[f].func))->getAddress();
        }
    }

    bool win64_backend::perform_register_allocation() const {
        return false;
    }

    void win64_backend::log_ir(bool do_log) {
        m_log_ir = do_log;
    }

    void win64_backend::generate_function(compilation_output& in, const compilation_output::func_def& f, llvm::Module* m) {
        using op = compile::operation;
        using var = compile::var;

        LLVMContext* ctx = m_llvm.getContext();

        robin_hood::unordered_map<u64, Value*> var_map;
        std::vector<Type*> ptypes;
        for (u8 a = 0;a < f.func->signature.arg_types.size();a++) {
            ptypes.push_back(llvm_type(f.func->signature.arg_types[a], ctx, m_types));
        }

        Function* func = m->getFunction(internal_func_name(f.func));

        BasicBlock* entry = BasicBlock::Create(*ctx, "entry", func);
        IRBuilder<> b(entry);
        
        Function* mdata = m->getFunction("__mdata");

        std::vector<Value*> call_params;

        for (u64 c = f.begin;c <= f.end;c++) {
            const compile::tac_instruction& i = in.code[c];
            const var& o1 = i.operands[0];
            const var& o2 = i.operands[1];
            const var& o3 = i.operands[2];
            Value* v1 = o1.valid() ? llvm_value(o1, var_map, func, ctx, m_types) : nullptr;
            Value* v2 = o2.valid() ? llvm_value(o2, var_map, func, ctx, m_types) : nullptr;
            Value* v3 = o3.valid() ? llvm_value(o3, var_map, func, ctx, m_types) : nullptr;

            switch (i.op) {
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
                    Value* addr = nullptr;

                    if (o3.is_imm()) {
                        // base address + absolute offset
                        addr = b.CreateAdd(v2, v3);
                    }
                    else addr = v2;

                    Value* ptr = b.CreateIntToPtr(addr, PointerType::get(llvm_type(o1.type(), ctx, m_types), 0));

                    var_map[o1.reg_id()] = b.CreateLoad(llvm_type(o1.type(), ctx, m_types), ptr);
                    break;
                }
                case op::store: {
                    Value* ptr = b.CreateIntToPtr(v1, PointerType::get(v2->getType(), 0));
                    b.CreateStore(v2, ptr);
                    break;
                }
                case op::stack_alloc: {
                    Value* ptr = b.CreateAlloca(Type::getInt8Ty(*ctx), v2);
                    var_map[o1.reg_id()] = b.CreatePtrToInt(ptr, Type::getInt64Ty(*ctx));
                    break;
                }
                case op::stack_free: {
                    break;
                }
                case op::module_data: {
                    script_module* mod = o2.imm_u() == in.mod->id() ? in.mod : m_ctx->module(o2.imm_u());
                    if (mod) {
                        auto f = METHOD_PTR(script_buffer, data, void*, u64);
                        Value* methodptr = ConstantInt::get(Type::getInt64Ty(*ctx), *(u64*)reinterpret_cast<void*>(&f), false);
                        Value* objptr = ConstantInt::get(Type::getInt64Ty(*ctx), (u64)mod->data(), false);
                        Value* offset = ConstantInt::get(Type::getInt64Ty(*ctx), o3.imm_u(), false);
                        var_map[o1.reg_id()] = b.CreateCall(mdata, {
                            methodptr,
                            objptr,
                            offset
                        });
                    } else {
                        // exception
                    }
                    break;
                }
                case op::iadd:
                case op::uadd: {
                    var_map[o1.reg_id()] = b.CreateAdd(v2, v3);
                    break;
                }
                case op::isub:
                case op::usub: {
                    var_map[o1.reg_id()] = b.CreateSub(v2, v3);
                    break;
                }
                case op::imul:
                case op::umul: {
                    var_map[o1.reg_id()] = b.CreateMul(v2, v3);
                    break;
                }
                case op::idiv: {
                    var_map[o1.reg_id()] = b.CreateSDiv(v2, v3);
                    break;
                }
                case op::imod: {
                    var_map[o1.reg_id()] = b.CreateSRem(v2, v3);
                    break;
                }
                case op::udiv: {
                    var_map[o1.reg_id()] = b.CreateUDiv(v2, v3);
                    break;
                }
                case op::umod: {
                    var_map[o1.reg_id()] = b.CreateURem(v2, v3);
                    break;
                }
                case op::fadd:
                case op::dadd: {
                    var_map[o1.reg_id()] = b.CreateFAdd(v2, v3);
                    break;
                }
                case op::fsub:
                case op::dsub: {
                    var_map[o1.reg_id()] = b.CreateFSub(v2, v3);
                    break;
                }
                case op::fmul:
                case op::dmul: {
                    var_map[o1.reg_id()] = b.CreateFMul(v2, v3);
                    break;
                }
                case op::fdiv:
                case op::ddiv: {
                    var_map[o1.reg_id()] = b.CreateFDiv(v2, v3);
                    break;
                }
                case op::fmod:
                case op::dmod: {
                    var_map[o1.reg_id()] = b.CreateFRem(v2, v3);
                    break;
                }
                case op::sl: {
                    var_map[o1.reg_id()] = b.CreateShl(v2, v3);
                    break;
                }
                case op::sr: {
                    var_map[o1.reg_id()] = b.CreateLShr(v2, v3);
                    break;
                }
                case op::land: {
                    var_map[o1.reg_id()] = b.CreateAnd(v2, v3);
                    break;
                }
                case op::lor: {
                    var_map[o1.reg_id()] = b.CreateOr(v2, v3);
                    break;
                }
                case op::band: {
                    Value* a0 = b.CreateIsNotNull(v2);
                    Value* a1 = b.CreateIsNotNull(v3);
                    var_map[o1.reg_id()] = b.CreateAnd(a0, a1);
                    break;
                }
                case op::bor: {
                    Value* a0 = b.CreateIsNotNull(v2);
                    Value* a1 = b.CreateIsNotNull(v3);
                    var_map[o1.reg_id()] = b.CreateOr(a0, a1);
                    break;
                }
                case op::bxor: {
                    var_map[o1.reg_id()] = b.CreateXor(v2, v3);
                    break;
                }
                case op::ilt: {
                    var_map[o1.reg_id()] = b.CreateICmpSLT(v2, v3);
                    break;
                }
                case op::igt: {
                    var_map[o1.reg_id()] = b.CreateICmpSGT(v2, v3);
                    break;
                }
                case op::ilte: {
                    var_map[o1.reg_id()] = b.CreateICmpSLE(v2, v3);
                    break;
                }
                case op::igte: {
                    var_map[o1.reg_id()] = b.CreateICmpSGE(v2, v3);
                    break;
                }
                case op::incmp:
                case op::uncmp: {
                    var_map[o1.reg_id()] = b.CreateICmpNE(v2, v3);
                    break;
                }
                case op::icmp:
                case op::ucmp: {
                    var_map[o1.reg_id()] = b.CreateICmpEQ(v2, v3);
                    break;
                }
                case op::ult: {
                    var_map[o1.reg_id()] = b.CreateICmpULT(v2, v3);
                    break;
                }
                case op::ugt: {
                    var_map[o1.reg_id()] = b.CreateICmpUGT(v2, v3);
                    break;
                }
                case op::ulte: {
                    var_map[o1.reg_id()] = b.CreateICmpULE(v2, v3);
                    break;
                }
                case op::ugte: {
                    var_map[o1.reg_id()] = b.CreateICmpUGE(v2, v3);
                    break;
                }
                case op::flt:
                case op::dlt: {
                    var_map[o1.reg_id()] = b.CreateFCmpOLT(v2, v3);
                    break;
                }
                case op::fgt:
                case op::dgt: {
                    var_map[o1.reg_id()] = b.CreateFCmpOGT(v2, v3);
                    break;
                }
                case op::flte:
                case op::dlte: {
                    var_map[o1.reg_id()] = b.CreateFCmpOLE(v2, v3);
                    break;
                }
                case op::fgte:
                case op::dgte: {
                    var_map[o1.reg_id()] = b.CreateFCmpOGE(v2, v3);
                    break;
                }
                case op::fncmp:
                case op::dncmp: {
                    var_map[o1.reg_id()] = b.CreateFCmpONE(v2, v3);
                    break;
                }
                case op::fcmp:
                case op::dcmp: {
                    var_map[o1.reg_id()] = b.CreateFCmpOEQ(v2, v3);
                    break;
                }
                case op::eq: {
                    var_map[o1.reg_id()] = v2;
                    break;
                }
                case op::neg: {
                    if (o1.type()->is_floating_point) {
                        var_map[o1.reg_id()] = b.CreateFNeg(v1);
                    } else {
                        var_map[o1.reg_id()] = b.CreateNeg(v1);
                    }
                    break;
                }
                case op::call: {
                    Function* f = m->getFunction(internal_func_name(i.callee));
                    if (f) {
                        if (i.callee->is_host) {
                            if (i.callee->access.wrapped->srv_wrapper_func) {
                                if (i.callee->is_method_of && !i.callee->is_static) {
                                    // return value pointer, call_method_func, func_ptr, call_params...
                                    call_params.insert(call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                                    call_params.insert(call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->call_method_func), false));
                                    call_params.insert(call_params.begin(), v1);
                                } else {
                                    // return value pointer, func_ptr, call_params...
                                    call_params.insert(call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                                    call_params.insert(call_params.begin(), v1);
                                }
                                b.CreateCall(f, call_params);
                            } else if (i.callee->access.wrapped->call_method_func) {
                                // func_ptr, call_params...
                                call_params.insert(call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                                if (o1.valid()) var_map[o1.reg_id()] = b.CreateCall(f, call_params);
                                else b.CreateCall(f, call_params);
                            } else {
                                if (o1.valid()) var_map[o1.reg_id()] = b.CreateCall(f, call_params);
                                else b.CreateCall(f, call_params);
                            }
                        } else {
                            if (o1.valid()) var_map[o1.reg_id()] = b.CreateCall(f, call_params);
                            else b.CreateCall(f, call_params);
                        }
                    }
                    call_params.clear();
                    break;
                }
                case op::param: {
                    call_params.push_back(v1);
                    break;
                }
                case op::ret: {
                    if (f.func->signature.return_type->size == 0) b.CreateRetVoid();
                    else b.CreateRet(v1);
                    break;
                }
                case op::cvt: {
                    script_type* from = m_ctx->module(extract_left_u32(o2.imm_u()))->types()->get(extract_right_u32(o2.imm_u()));
                    script_type* to = m_ctx->module(extract_left_u32(o3.imm_u()))->types()->get(extract_right_u32(o3.imm_u()));
                    Value* o = nullptr;
                    Type* t = llvm_type(to, ctx, m_types);

                    if (from->is_floating_point) {
                        if (to->is_floating_point) {
                            if (from->size != to->size) {
                                if (from->size > to->size) o = b.CreateFPTrunc(v1, t);
                                else o = b.CreateFPExt(v1, t);
                            }
                        } else {
                            if (to->is_unsigned) o = b.CreateFPToUI(v1, t);
                            else o = b.CreateFPToSI(v1, t);
                        }
                    } else {
                        if (to->is_floating_point) {
                            if (from->is_unsigned) o = b.CreateUIToFP(v1, t);
                            else o = b.CreateSIToFP(v1, t);
                        } else {
                            if (from->is_unsigned) {
                                if (to->is_unsigned && from->size != to->size) {
                                    if (from->size > to->size) o = b.CreateTrunc(v1, t);
                                    else o = b.CreateZExt(v1, t);
                                }
                                else {
                                    // ui -> si (probably wrong, couldn't find any info on what to do)
                                    if (from->size != to->size) {
                                        if (from->size > to->size) o = b.CreateTrunc(v1, t);
                                        else o = b.CreateZExt(v1, t);
                                    }
                                }
                            } else {
                                if (!to->is_unsigned && from->size != to->size) {
                                    if (from->size > to->size) o = b.CreateTrunc(v1, t);
                                    else o = b.CreateSExt(v1, t);
                                }
                                else {
                                    // si -> ui (probably wrong, couldn't find any info on what to do)
                                    if (from->size != to->size) {
                                        if (from->size > to->size) o = b.CreateTrunc(v1, t);
                                        else o = b.CreateSExt(v1, t);
                                    }
                                }
                            }
                        }
                    }

                    if (o) var_map[o1.reg_id()] = o;

                    break;
                }
                case op::branch: {
                    break;
                }
                case op::jump: {
                    break;
                }
            }
        }
    }

    void win64_backend::add_bindings_to_module(llvm::Module* m) {
        LLVMContext& ctx = *m_llvm.getContext();
        Function* mdata = Function::Create(
            FunctionType::get(
                Type::getInt64Ty(ctx),
                { Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx) },
                false
            ),
            GlobalValue::LinkageTypes::ExternalLinkage,
            0,
            "__mdata",
            m
        );
        
        auto global_funcs = m_ctx->global()->functions();
        for (u32 f = 0;f < global_funcs.size();f++) {
            script_function* func = global_funcs[f];
            std::vector<Type*> params;

            Type* ret = llvm_type(global_funcs[f]->signature.return_type, &ctx, m_types);

            if (func->access.wrapped->srv_wrapper_func) {
                if (func->is_method_of && !func->is_static) {
                    // return value pointer, call_method_func, func_ptr, params...
                    params.push_back(Type::getInt64Ty(ctx));
                    params.push_back(Type::getInt64Ty(ctx));
                    params.push_back(Type::getInt64Ty(ctx));
                    ret = Type::getVoidTy(ctx);
                } else {
                    // return value pointer, func_ptr, params...
                    params.push_back(Type::getInt64Ty(ctx));
                    params.push_back(Type::getInt64Ty(ctx));
                    ret = Type::getVoidTy(ctx);
                }
            } else if (func->access.wrapped->call_method_func) {
                // func_ptr, params...
                params.push_back(Type::getInt64Ty(ctx));
            }

            for (u8 a = 0;a < global_funcs[f]->signature.arg_types.size();a++) {
                params.push_back(llvm_type(global_funcs[f]->signature.arg_types[a], &ctx, m_types));
            }

            FunctionType* sig = FunctionType::get(ret, params, false);
            Function::Create(sig, GlobalValue::LinkageTypes::ExternalLinkage, 0, internal_func_name(global_funcs[f]), m);
        }
    }

    void win64_backend::call(script_function* func, void* ret, void** args) {
        DCCallVM* cvm = m_ctx->call_vm();
        if (func->is_host) {
            func->access.wrapped->call(cvm, ret, args);
            return;
        }

        dcMode(cvm, DC_CALL_C_DEFAULT);
        dcReset(cvm);

        for (u8 a = 0;a < func->signature.arg_types.size();a++) {
            script_type* tp = func->signature.arg_types[a];
            if (tp->is_primitive) {
                if (tp->is_floating_point) {
                    if (tp->size == sizeof(f64)) dcArgDouble(cvm, *(f64*)&args[a]);
                    else dcArgFloat(cvm, *(f32*)&args[a]);
                } else {
                    switch (tp->size) {
                        case sizeof(i8): {
                            if (tp->name == "bool") dcArgBool(cvm, *(bool*)&args[a]);
                            else dcArgChar(cvm, *(i8*)&args[a]);
                            break;
                        }
                        case sizeof(i16): { dcArgShort(cvm, *(i16*)&args[a]); break; }
                        case sizeof(i32): {
                            if (tp->is_unsigned) dcArgLong(cvm, *(u32*)&args[a]);
                            else dcArgInt(cvm, *(i32*)&args[a]);
                            break;
                        }
                        case sizeof(i64): { dcArgLongLong(cvm, *(i64*)&args[a]); break; }
                    }
                }
            } else {
                dcArgPointer(cvm, args[a]);
            }
        }

        script_type* rtp = func->signature.return_type;
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
                // uh oh, returns a struct
            }
        }
    }

    std::string win64_backend::internal_func_name(script_function* f) {
        return format("%s_%llX", f->name.c_str(), reinterpret_cast<u64>(f));
    }
};