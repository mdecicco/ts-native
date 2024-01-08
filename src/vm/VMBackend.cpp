#include <tsn/vm/VMBackend.h>
#include <tsn/vm/VM.h>
#include <tsn/vm/Instruction.h>
#include <tsn/vm/types.h>
#include <tsn/vm/macros.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/backend/RegisterAllocationStep.h>
#include <tsn/backend/StackManager.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/compiler/Output.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>

#include <utils/robin_hood.h>
#include <utils/Array.hpp>
#include <utils/PerThreadObject.hpp>

#include <assert.h>

namespace tsn {
    namespace vm {
        using namespace compiler;
        using namespace ffi;
        using op = ir_instruction;
        using vmi = vm_instruction;
        using vmr = vm_register;
        using var = Value;

        void getArgRegisters(FunctionType* sig, utils::Array<arg_location>& out) {
            vm_register nextFP = vm_register::fa0;
            vm_register nextGP = vm_register::a0;
            u32 sp_offset = 0;

            const auto& args = sig->getArguments();
            for (u32 a = 0;a < args.size();a++) {
                const auto& ti = args[a].dataType->getInfo();
                if (ti.is_floating_point && args[a].argType == arg_type::value) {
                    if (nextFP > vm_register::fa7) {
                        u32 size = (u32)sizeof(void*);
                        if (ti.is_primitive) size = ti.size;

                        out.push({ vm_register::sp, sp_offset, size });
                        sp_offset += size;

                        continue;
                    }

                    out.push({ nextFP, 0, 0 });
                    nextFP = vm_register(u32(nextFP) + 1);
                } else {
                    if (nextGP > vm_register::a7) {
                        u32 size = (u32)sizeof(void*);
                        if (ti.is_primitive) size = ti.size;

                        out.push({ vm_register::sp, sp_offset, size });
                        sp_offset += size;

                        continue;
                    }

                    out.push({ nextGP, 0, 0 });
                    nextGP = vm_register(u32(nextGP) + 1);
                }
            }
        }

        Backend::Backend(Context* ctx, u32 stackSize) : IBackend(ctx) {
            m_vms = new utils::PerThreadObject<VM>([ctx, stackSize]() {
                return new VM(ctx, stackSize);
            });
            
            m_instructions.push(encode(vmi::term));
            m_map.add(0, 0, 0);
        }

        Backend::~Backend() {
        }
        
        void Backend::beforeCompile(Pipeline* input) {
            input->addOptimizationStep(new backend::RegisterAllocatonStep(m_ctx, 16, 16), true);
        }

        void Backend::generate(Pipeline* input) {
            m_map.setSource(input->getCompilerOutput()->getModule()->getSource());
            auto& functions = input->getCompilerOutput()->getCode();
            for (u32 i = 0;i < functions.size();i++) {
                generate(functions[i]);
            }
            m_map.setSource(nullptr);
        }

        void Backend::calcArgLifetimesAndBackupRegs(
            const utils::Array<arg_location>& argLocs,
            compiler::CodeHolder* ch,
            robin_hood::unordered_flat_set<vm_register>& assignedNonVolatile,
            robin_hood::unordered_map<vm_register, address>& argLifetimeEnds
        ) {
            for (address c = 0;c < ch->code.size();c++) {
                auto& i = ch->code[c]; 

                // Extend any relevant argument lifetimes
                for (u8 o = 0;o < i.oCnt;o++) {
                    if (i.operands[o].isArg()) {
                        u32 argIdx = i.operands[o].getImm<u32>();
                        if (argLocs[argIdx].reg_id == vm_register::sp) continue;
                        argLifetimeEnds[argLocs[argIdx].reg_id] = c;
                    }
                }

                // If a backwards jump goes into a live range,
                // then that live range must be extended to fit
                // the jump (if it doesn't already)

                if (i.op == op::ir_jump) {
                    address jaddr = ch->labels.get(i.operands[0].getImm<label_id>());
                    if (jaddr > c) continue;
                    for (auto& r : argLifetimeEnds) {
                        if (r.second >= jaddr && r.second < c) r.second = c;
                    }
                } else if (i.op == op::ir_branch) {
                    address taddr = ch->labels.get(i.operands[1].getImm<label_id>());
                    address faddr = ch->labels.get(i.operands[2].getImm<label_id>());
                    if (taddr > c && faddr > c) continue;
                    for (auto& r : argLifetimeEnds) {
                        if (r.second >= taddr && r.second < taddr) r.second = c;
                        if (r.second >= faddr && r.second < faddr) r.second = c;
                    }
                }

                const var* assigns = i.assigns();
                if (assigns) {
                    vmr assignedReg = vmr::register_count;
                    if (assigns->isStack()) {
                        assignedReg = vmr(u16(vmr::v0) + instruction_info(i.op).assigns_operand_index);
                    } else if (assigns->isValid() && !assigns->isImm()) {
                        if (assigns->isArg()) {
                            assignedReg = argLocs[assigns->getImm<u32>()].reg_id;
                            if (assignedReg == vm_register::sp) continue;
                        } else {
                            assignedReg = vmr((u16)vmr::s0 + (assigns->getRegId() - 1));
                            if (assigns->isFloatingPoint()) assignedReg = vmr((u16)vmr::f0 + (assigns->getRegId() - 1));
                        }
                    }

                    if (!vm_reg_is_volatile(assignedReg)) assignedNonVolatile.insert(assignedReg);
                } else if (i.op == op::ir_call) {
                    assignedNonVolatile.insert(vmr::ra);
                    FunctionType* sig = (FunctionType*)i.operands[0].getType();
                    
                    const auto& calleeArgs = sig->getArguments();
                    utils::Array<arg_location> calleeArgLocs;
                    getArgRegisters(sig, calleeArgLocs);

                    for (u8 a = 0;a < calleeArgs.size();a++) {
                        if (calleeArgLocs[a].reg_id == vm_register::sp) continue;

                        bool alreadyOwned = false;
                        for (u8 a1 = 0;a1 < argLocs.size();a1++) {
                            if (calleeArgLocs[a].reg_id == argLocs[a1].reg_id) {
                                alreadyOwned = true;
                                break;
                            }
                        }

                        // caller of this function already backed up this register
                        if (alreadyOwned) continue;
                        assignedNonVolatile.insert(calleeArgLocs[a].reg_id);
                    }
                }
            }
        }
        
        void Backend::generate(CodeHolder* ch) {
            DataType* v2f = m_ctx->getTypes()->getVec2f();
            DataType* v2d = m_ctx->getTypes()->getVec2d();
            DataType* v3f = m_ctx->getTypes()->getVec3f();
            DataType* v3d = m_ctx->getTypes()->getVec3d();
            DataType* v4f = m_ctx->getTypes()->getVec4f();
            DataType* v4d = m_ctx->getTypes()->getVec4d();

            robin_hood::unordered_map<alloc_id, u64> stack_addrs;
            robin_hood::unordered_map<label_id, address> label_map;

            struct deferred_label { address idx; label_id label; }; // label == 0 indicates a jump to the function epilogue
            utils::Array<deferred_label> deferred_label_instrs;

            // determine registers that need to be handled by the prologue and epilogue
            robin_hood::unordered_flat_set<vm_register> assigned_non_volatile;

            // also the end of the lifetimes of the argument registers
            robin_hood::unordered_map<vm_register, address> arg_lifetime_ends;

            utils::Array<var> params;
            ffi::Function* cf = ch->owner;

            auto& code = ch->code;
            address out_begin = m_instructions.size();

            // Calculate argument registers for the current function
            utils::Array<arg_location> argLocs;
            getArgRegisters(cf->getSignature(), argLocs);

            calcArgLifetimesAndBackupRegs(argLocs, ch, assigned_non_volatile, arg_lifetime_ends);

            backend::StackManager stack;
            
            // allocate stack slots for stack arguments (Don't need to be freed because they remain live until the function epilogue)
            for (u32 i = 0;i < argLocs.size();i++) {
                if (argLocs[i].reg_id == vmr::sp) {
                    u32 addr = stack.alloc(argLocs[i].stack_size);
                    assert(addr != argLocs[i].stack_offset);
                }
            }

            // generate function prologue
            struct prologueInfo { vmr reg; u32 addr; };
            utils::Array<prologueInfo> prologue;
            for (auto r : assigned_non_volatile) {
                u32 addr = stack.alloc(sizeof(u64));
                prologue.push({ r, addr });
                m_instructions.push(encode(vmi::st64).operand(r).operand(vmr::sp).operand(u64(addr)));
                const auto& src = cf->getSource();
                m_map.add(src.getLine(), src.getCol(), src.getLength());
            }

            for (address c = 0;c < code.size();c++) {
                const auto& i = code[c];
                const auto& iinfo = instruction_info(i.op);

                const var& o1 = i.operands[0];
                const var& o2 = i.operands[1];
                const var& o3 = i.operands[2];

                vmr r1;
                vmr r2;
                vmr r3;
                DataType* t1 = o1.isValid() ? o1.getType() : nullptr;
                DataType* t2 = o2.isValid() ? o2.getType() : nullptr;
                DataType* t3 = o3.isValid() ? o3.getType() : nullptr;

                auto arith = [&](vmi rr, vmi ri, vmi ir) {
                    if (o3.isImm()) {
                        if (t3->getInfo().is_floating_point) {
                            if (t3->getInfo().size == sizeof(f64)) m_instructions.push(encode(ri).operand(r1).operand(r2).operand(o3.getImm<f64>()));
                            else m_instructions.push(encode(ri).operand(r1).operand(r2).operand(o3.getImm<f32>()));
                        } else {
                            if (t3->getInfo().is_unsigned) m_instructions.push(encode(ri).operand(r1).operand(r2).operand(o3.getImm<u64>()));
                            else m_instructions.push(encode(ri).operand(r1).operand(r2).operand(o3.getImm<i64>()));
                        }
                    } else if (o2.isImm()) {
                        if (t2->getInfo().is_floating_point) {
                            if (t2->getInfo().size == sizeof(f64)) m_instructions.push(encode(ir).operand(r1).operand(r3).operand(o2.getImm<f64>()));
                            else m_instructions.push(encode(ir).operand(r1).operand(r3).operand(o2.getImm<f32>()));
                        } else {
                            if (t2->getInfo().is_unsigned) m_instructions.push(encode(ir).operand(r1).operand(r3).operand(o2.getImm<u64>()));
                            else m_instructions.push(encode(ir).operand(r1).operand(r3).operand(o2.getImm<i64>()));
                        }
                    } else m_instructions.push(encode(rr).operand(r1).operand(r2).operand(r3));
                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                };

                // Determine operand register IDs
                if (i.op != op::ir_param) {
                    if (o1.isStack()) {
                        if (!i.assigns()) {
                            r1 = vmr::v0;
                            if (t1->getInfo().is_floating_point) r1 = vmr::vf0;

                            u8 sz = t1->getInfo().size;
                            if (!t1->getInfo().is_primitive) sz = sizeof(void*);
                            vmi ld = vmi::ld8;
                            switch (sz) {
                                case 2: { ld = vmi::ld16; break; }
                                case 4: { ld = vmi::ld32; break; }
                                case 8: { ld = vmi::ld64; break; }
                                default: { break; }
                            }
                            m_instructions.push(encode(ld).operand(r1).operand(vmr::sp).operand(stack_addrs[o1.getStackAllocId()]));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        } else r1 = vmr::v0;
                    } else if (o1.isValid() && !o1.isImm()) {
                        if (o1.isArg()) {
                            r1 = argLocs[o1.getImm<u32>()].reg_id;
                            if (r1 == vmr::sp) {
                                // arg is in the stack
                                u32 offset = argLocs[o1.getImm<u32>()].stack_offset;
                                    
                                r1 = vmr::v0;
                                if (t1->getInfo().is_floating_point) r1 = vmr::vf0;

                                u8 sz = t1->getInfo().size;
                                if (!t1->getInfo().is_primitive) sz = sizeof(void*);
                                vmi ld = vmi::ld8;
                                switch (sz) {
                                    case 2: { ld = vmi::ld16; break; }
                                    case 4: { ld = vmi::ld32; break; }
                                    case 8: { ld = vmi::ld64; break; }
                                    default: { break; }
                                }
                                m_instructions.push(encode(ld).operand(r1).operand(vmr::sp).operand((u64)offset));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        } else {
                            r1 = vmr((u8)vmr::s0 + (o1.getRegId() - 1));
                            if (o1.isFloatingPoint()) r1 = vmr((u8)vmr::f0 + (o1.getRegId() - 1));
                        }
                    }

                    if (o2.isStack()) {
                        r2 = vmr::v1;
                        if (t2->getInfo().is_floating_point) r2 = vmr::vf1;

                        u8 sz = t2->getInfo().size;
                        if (!t2->getInfo().is_primitive) sz = sizeof(void*);
                        vmi ld = vmi::ld8;
                        switch (sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: { break; }
                        }
                        m_instructions.push(encode(ld).operand(r2).operand(vmr::sp).operand(stack_addrs[o2.getStackAllocId()]));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                    } else if (o2.isValid() && !o2.isImm()) {
                        if (o2.isArg()) {
                            r2 = argLocs[o2.getImm<u32>()].reg_id;
                            if (r2 == vmr::sp) {
                                // arg is in the stack
                                u32 offset = argLocs[o2.getImm<u32>()].stack_offset;
                                    
                                r2 = vmr::v1;
                                if (t2->getInfo().is_floating_point) r2 = vmr::vf1;

                                u8 sz = t2->getInfo().size;
                                if (!t2->getInfo().is_primitive) sz = sizeof(void*);
                                vmi ld = vmi::ld8;
                                switch (sz) {
                                    case 2: { ld = vmi::ld16; break; }
                                    case 4: { ld = vmi::ld32; break; }
                                    case 8: { ld = vmi::ld64; break; }
                                    default: { break; }
                                }
                                m_instructions.push(encode(ld).operand(r2).operand(vmr::sp).operand((u64)offset));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        } else {
                            r2 = vmr((u8)vmr::s0 + (o2.getRegId() - 1));
                            if (o2.isFloatingPoint()) r2 = vmr((u8)vmr::f0 + (o2.getRegId() - 1));
                        }
                    }

                    if (o3.isStack()) {
                        r3 = vmr::v2;
                        if (t3->getInfo().is_floating_point) r3 = vmr::vf2;

                        u8 sz = t3->getInfo().size;
                        if (!o3.getType()->getInfo().is_primitive) sz = sizeof(void*);
                        vmi ld = vmi::ld8;
                        switch (sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: { break; }
                        }
                        m_instructions.push(encode(ld).operand(r3).operand(vmr::sp).operand(stack_addrs[o3.getStackAllocId()]));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                    } else if (o3.isValid() && !o3.isImm()) {
                        if (o3.isArg()) {
                            r3 = argLocs[o3.getImm<u32>()].reg_id;
                            if (r3 == vmr::sp) {
                                // arg is in the stack
                                u32 offset = argLocs[o3.getImm<u32>()].stack_offset;
                                    
                                r3 = vmr::v2;
                                if (t2->getInfo().is_floating_point) r3 = vmr::vf2;

                                u8 sz = t3->getInfo().size;
                                if (!t3->getInfo().is_primitive) sz = sizeof(void*);
                                vmi ld = vmi::ld8;
                                switch (sz) {
                                    case 2: { ld = vmi::ld16; break; }
                                    case 4: { ld = vmi::ld32; break; }
                                    case 8: { ld = vmi::ld64; break; }
                                    default: { break; }
                                }
                                m_instructions.push(encode(ld).operand(r3).operand(vmr::sp).operand((u64)offset));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        } else {
                            r3 = vmr((u8)vmr::s0 + (o3.getRegId() - 1));
                            if (o3.isFloatingPoint()) r3 = vmr((u8)vmr::f0 + (o3.getRegId() - 1));
                        }
                    }
                }

                // This comment was written in a police station while waiting for a ride
                switch (i.op) {
                    case op::ir_noop: {
                        // m_instructions.push(encode(vmi::null));
                        // m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_load: {
                        u8 sz = o1.getType()->getInfo().size;
                        if (!o1.getType()->getInfo().is_primitive || o1.getFlags().is_pointer) sz = sizeof(void*);
                        vmi ld = vmi::ld8;
                        switch (sz) {
                            case 2: { ld = vmi::ld16; break; }
                            case 4: { ld = vmi::ld32; break; }
                            case 8: { ld = vmi::ld64; break; }
                            default: {
                                break;
                            }
                        }

                        u64 off = 0;
                        if (o3.isValid() && o3.isImm()) off = o3.getImm<u64>();

                        m_instructions.push(encode(ld).operand(r1).operand(r2).operand(off));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_store: {
                        if (o1.isImm()) {
                            if (o1.getImm<u64>() == 0) {
                                r1 = vmr::zero;
                            } else {
                                if (t1->getInfo().is_floating_point) {
                                    r1 = vmr::vf3;
                                    if (t1->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::daddi).operand(r1).operand(vmr::zero).operand(o1.getImm<f64>()));
                                    else m_instructions.push(encode(vmi::faddi).operand(r1).operand(vmr::zero).operand(o1.getImm<f32>()));
                                } else {
                                    r1 = vmr::v3;
                                    m_instructions.push(encode(vmi::addui).operand(r1).operand(vmr::zero).operand(o1.getImm<u64>()));
                                }
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        }
                        
                        u8 sz = t1->getInfo().size;
                        if (!t1->getInfo().is_primitive || o1.getFlags().is_pointer) sz = sizeof(void*);
                        vmi st = vmi::st8;
                        switch (sz) {
                            case 2: { st = vmi::st16; break; }
                            case 4: { st = vmi::st32; break; }
                            case 8: { st = vmi::st64; break; }
                            default: { break; }
                        }

                        u64 off = 0;
                        if (o3.isValid() && o3.isImm()) off = o3.getImm<u64>();

                        m_instructions.push(encode(st).operand(r1).operand(r2).operand(off));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_stack_allocate: {
                        u64 addr = stack.alloc(o1.getImm<u32>());
                        stack_addrs[o2.getImm<alloc_id>()] = addr;
                        break;
                    }
                    case op::ir_stack_ptr: {
                        u64 addr = stack_addrs[o2.getImm<alloc_id>()];
                        m_instructions.push(encode(vmi::addui).operand(r1).operand(vmr::sp).operand(addr));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_stack_free: {
                        auto it = stack_addrs.find(o1.getImm<alloc_id>());
                        stack.free((u32)it->second);
                        stack_addrs.erase(it);
                        break;
                    }
                    case op::ir_module_data: {
                        Module* m = m_ctx->getModule(o2.getImm<u32>());
                        void* ptr = m->getDataInfo(o3.getImm<u32>()).ptr;
                        m_instructions.push(encode(vmi::addui).operand(r1).operand(vmr::zero).operand(reinterpret_cast<u64>(ptr)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vset: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fsetsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dsetsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fsetsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dsetsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fsetsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dsetsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fsets).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dsets).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fsets).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dsets).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fsets).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dsets).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fset).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dset).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fset).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dset).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fset).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dset).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vadd: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2faddsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2daddsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3faddsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3daddsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4faddsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4daddsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fadds).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dadds).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fadds).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dadds).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fadds).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dadds).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fadd).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dadd).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fadd).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dadd).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fadd).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dadd).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vsub: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fsubsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dsubsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fsubsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dsubsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fsubsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dsubsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fsubs).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dsubs).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fsubs).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dsubs).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fsubs).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dsubs).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fsub).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dsub).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fsub).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dsub).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fsub).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dsub).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vmul: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmulsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmulsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmulsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmulsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmulsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmulsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmuls).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmuls).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmuls).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmuls).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmuls).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmuls).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmul).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmul).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmul).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmul).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmul).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmul).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vdiv: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fdivsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2ddivsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fdivsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3ddivsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fdivsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4ddivsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fdivs).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2ddivs).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fdivs).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3ddivs).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fdivs).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4ddivs).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fdiv).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2ddiv).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fdiv).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3ddiv).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fdiv).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4ddiv).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vmod: {
                        if (o2.isImm()) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmodsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmodsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmodsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmodsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmodsi).operand(r1).operand(o2.getImm<f64>()));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmodsi).operand(r1).operand(o2.getImm<f64>()));
                        } else if (t2->getInfo().is_primitive) {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmods).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmods).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmods).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmods).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmods).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmods).operand(r1).operand(r2));
                        } else {
                            if (t1 == v2f) m_instructions.push(encode(vmi::v2fmod).operand(r1).operand(r2));
                            else if (t1 == v2d) m_instructions.push(encode(vmi::v2dmod).operand(r1).operand(r2));
                            else if (t1 == v3f) m_instructions.push(encode(vmi::v3fmod).operand(r1).operand(r2));
                            else if (t1 == v3d) m_instructions.push(encode(vmi::v3dmod).operand(r1).operand(r2));
                            else if (t1 == v4f) m_instructions.push(encode(vmi::v4fmod).operand(r1).operand(r2));
                            else if (t1 == v4d) m_instructions.push(encode(vmi::v4dmod).operand(r1).operand(r2));
                        }
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vneg: {
                        if (t1 == v2f) m_instructions.push(encode(vmi::v2fneg).operand(r1));
                        else if (t1 == v2d) m_instructions.push(encode(vmi::v2dneg).operand(r1));
                        else if (t1 == v3f) m_instructions.push(encode(vmi::v3fneg).operand(r1));
                        else if (t1 == v3d) m_instructions.push(encode(vmi::v3dneg).operand(r1));
                        else if (t1 == v4f) m_instructions.push(encode(vmi::v4fneg).operand(r1));
                        else if (t1 == v4d) m_instructions.push(encode(vmi::v4dneg).operand(r1));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vdot: {
                        if (t2 == v2f) m_instructions.push(encode(vmi::v2fdot).operand(r1).operand(r2).operand(r3));
                        else if (t2 == v2d) m_instructions.push(encode(vmi::v2ddot).operand(r1).operand(r2).operand(r3));
                        else if (t2 == v3f) m_instructions.push(encode(vmi::v3fdot).operand(r1).operand(r2).operand(r3));
                        else if (t2 == v3d) m_instructions.push(encode(vmi::v3ddot).operand(r1).operand(r2).operand(r3));
                        else if (t2 == v4f) m_instructions.push(encode(vmi::v4fdot).operand(r1).operand(r2).operand(r3));
                        else if (t2 == v4d) m_instructions.push(encode(vmi::v4ddot).operand(r1).operand(r2).operand(r3));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vmag: {
                        if (t2 == v2f) m_instructions.push(encode(vmi::v2fmag).operand(r1).operand(r2));
                        else if (t2 == v2d) m_instructions.push(encode(vmi::v2dmag).operand(r1).operand(r2));
                        else if (t2 == v3f) m_instructions.push(encode(vmi::v3fmag).operand(r1).operand(r2));
                        else if (t2 == v3d) m_instructions.push(encode(vmi::v3dmag).operand(r1).operand(r2));
                        else if (t2 == v4f) m_instructions.push(encode(vmi::v4fmag).operand(r1).operand(r2));
                        else if (t2 == v4d) m_instructions.push(encode(vmi::v4dmag).operand(r1).operand(r2));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vmagsq: {
                        if (t2 == v2f) m_instructions.push(encode(vmi::v2fmagsq).operand(r1).operand(r2));
                        else if (t2 == v2d) m_instructions.push(encode(vmi::v2dmagsq).operand(r1).operand(r2));
                        else if (t2 == v3f) m_instructions.push(encode(vmi::v3fmagsq).operand(r1).operand(r2));
                        else if (t2 == v3d) m_instructions.push(encode(vmi::v3dmagsq).operand(r1).operand(r2));
                        else if (t2 == v4f) m_instructions.push(encode(vmi::v4fmagsq).operand(r1).operand(r2));
                        else if (t2 == v4d) m_instructions.push(encode(vmi::v4dmagsq).operand(r1).operand(r2));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vnorm: {
                        if (t1 == v2f) m_instructions.push(encode(vmi::v2fnorm).operand(r1));
                        else if (t1 == v2d) m_instructions.push(encode(vmi::v2dnorm).operand(r1));
                        else if (t1 == v3f) m_instructions.push(encode(vmi::v3fnorm).operand(r1));
                        else if (t1 == v3d) m_instructions.push(encode(vmi::v3dnorm).operand(r1));
                        else if (t1 == v4f) m_instructions.push(encode(vmi::v4fnorm).operand(r1));
                        else if (t1 == v4d) m_instructions.push(encode(vmi::v4dnorm).operand(r1));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_vcross: {
                        if (t1 == v3f) m_instructions.push(encode(vmi::v3fcross).operand(r1).operand(r2).operand(r3));
                        else if (t1 == v3d) m_instructions.push(encode(vmi::v3dcross).operand(r1).operand(r2).operand(r3));
                        else if (t1 == v4f) m_instructions.push(encode(vmi::v4fcross).operand(r1).operand(r2).operand(r3));
                        else if (t1 == v4d) m_instructions.push(encode(vmi::v4dcross).operand(r1).operand(r2).operand(r3));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_iadd: {
                        arith(vmi::add, vmi::addi, vmi::addi);
                        break;
                    }
                    case op::ir_isub: {
                        arith(vmi::sub, vmi::subi, vmi::subir);
                        break;
                    }
                    case op::ir_imul: {
                        arith(vmi::mul, vmi::muli, vmi::muli);
                        break;
                    }
                    case op::ir_idiv: {
                        arith(vmi::div, vmi::divi, vmi::divir);
                        break;
                    }
                    case op::ir_imod: {
                        break;
                    }
                    case op::ir_iinc: {
                        m_instructions.push(encode(vmi::addi).operand(r1).operand(r1).operand(i64(1)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_idec: {
                        m_instructions.push(encode(vmi::subi).operand(r1).operand(r1).operand(i64(1)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_uadd: {
                        arith(vmi::addu, vmi::addui, vmi::addui);
                        break;
                    }
                    case op::ir_usub: {
                        arith(vmi::subu, vmi::subui, vmi::subuir);
                        break;
                    }
                    case op::ir_umul: {
                        arith(vmi::mulu, vmi::mului, vmi::mului);
                        break;
                    }
                    case op::ir_udiv: {
                        arith(vmi::divu, vmi::divui, vmi::divuir);
                        break;
                    }
                    case op::ir_umod: {
                        break;
                    }
                    case op::ir_uinc: {
                        m_instructions.push(encode(vmi::addui).operand(r1).operand(r1).operand(u64(1)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_udec: {
                        m_instructions.push(encode(vmi::subui).operand(r1).operand(r1).operand(u64(1)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_fadd: {
                        arith(vmi::fadd, vmi::faddi, vmi::faddi);
                        break;
                    }
                    case op::ir_fsub: {
                        arith(vmi::fsub, vmi::fsubi, vmi::fsubir);
                        break;
                    }
                    case op::ir_fmul: {
                        arith(vmi::fmul, vmi::fmuli, vmi::fmuli);
                        break;
                    }
                    case op::ir_fdiv: {
                        arith(vmi::fdiv, vmi::fdivi, vmi::fdivir);
                        break;
                    }
                    case op::ir_fmod: {
                        break;
                    }
                    case op::ir_finc: {
                        m_instructions.push(encode(vmi::faddi).operand(r1).operand(r1).operand(f32(1.0f)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_fdec: {
                        m_instructions.push(encode(vmi::fsubi).operand(r1).operand(r1).operand(f32(1.0f)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_dadd: {
                        arith(vmi::dadd, vmi::daddi, vmi::daddi);
                        break;
                    }
                    case op::ir_dsub: {
                        arith(vmi::dsub, vmi::dsubi, vmi::dsubir);
                        break;
                    }
                    case op::ir_dmul: {
                        arith(vmi::dmul, vmi::dmuli, vmi::dmuli);
                        break;
                    }
                    case op::ir_ddiv: {
                        arith(vmi::ddiv, vmi::ddivi, vmi::ddivir);
                        break;
                    }
                    case op::ir_dmod: {
                        break;
                    }
                    case op::ir_dinc: {
                        m_instructions.push(encode(vmi::daddi).operand(r1).operand(r1).operand(f64(1.0)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_ddec: {
                        m_instructions.push(encode(vmi::dsubi).operand(r1).operand(r1).operand(f64(1.0)));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_shl: {
                        arith(vmi::sl, vmi::sli, vmi::slir);
                        break;
                    }
                    case op::ir_shr: {
                        arith(vmi::sr, vmi::sri, vmi::srir);
                        break;
                    }
                    case op::ir_land: {
                        arith(vmi::_and, vmi::andi, vmi::andi);
                        break;
                    }
                    case op::ir_lor: {
                        arith(vmi::_or, vmi::ori, vmi::ori);
                        break;
                    }
                    case op::ir_band: {
                        arith(vmi::band, vmi::bandi, vmi::bandi);
                        break;
                    }
                    case op::ir_bor: {
                        arith(vmi::bor, vmi::bori, vmi::bori);
                        break;
                    }
                    case op::ir_xor: {
                        arith(vmi::_xor, vmi::xori, vmi::xori);
                        break;
                    }
                    case op::ir_ilt: {
                        arith(vmi::lt, vmi::lti, vmi::gtei);
                        break;
                    }
                    case op::ir_igt: {
                        arith(vmi::gt, vmi::gti, vmi::ltei);
                        break;
                    }
                    case op::ir_ilte: {
                        arith(vmi::lte, vmi::ltei, vmi::gti);
                        break;
                    }
                    case op::ir_igte: {
                        arith(vmi::gte, vmi::gtei, vmi::lt);
                        break;
                    }
                    case op::ir_ineq: {
                        arith(vmi::ncmp, vmi::ncmpi, vmi::ncmpi);
                        break;
                    }
                    case op::ir_ieq: {
                        arith(vmi::cmp, vmi::cmpi, vmi::cmpi);
                        break;
                    }
                    case op::ir_ult: {
                        arith(vmi::lt, vmi::lti, vmi::gtei);
                        break;
                    }
                    case op::ir_ugt: {
                        arith(vmi::gt, vmi::gti, vmi::ltei);
                        break;
                    }
                    case op::ir_ulte: {
                        arith(vmi::lte, vmi::ltei, vmi::gti);
                        break;
                    }
                    case op::ir_ugte: {
                        arith(vmi::gte, vmi::gtei, vmi::lt);
                        break;
                    }
                    case op::ir_uneq: {
                        arith(vmi::ncmp, vmi::ncmpi, vmi::ncmpi);
                        break;
                    }
                    case op::ir_ueq: {
                        arith(vmi::cmp, vmi::cmpi, vmi::cmpi);
                        break;
                    }
                    case op::ir_flt: {
                        arith(vmi::flt, vmi::flti, vmi::fgtei);
                        break;
                    }
                    case op::ir_fgt: {
                        arith(vmi::fgt, vmi::fgti, vmi::fltei);
                        break;
                    }
                    case op::ir_flte: {
                        arith(vmi::flte, vmi::fltei, vmi::fgti);
                        break;
                    }
                    case op::ir_fgte: {
                        arith(vmi::fgte, vmi::fgtei, vmi::flt);
                        break;
                    }
                    case op::ir_fneq: {
                        arith(vmi::fncmp, vmi::fncmpi, vmi::fncmpi);
                        break;
                    }
                    case op::ir_feq: {
                        arith(vmi::fcmp, vmi::fcmpi, vmi::fcmpi);
                        break;
                    }
                    case op::ir_dlt: {
                        arith(vmi::dlt, vmi::dlti, vmi::dgtei);
                        break;
                    }
                    case op::ir_dgt: {
                        arith(vmi::dgt, vmi::dgti, vmi::dltei);
                        break;
                    }
                    case op::ir_dlte: {
                        arith(vmi::dlte, vmi::dltei, vmi::dgti);
                        break;
                    }
                    case op::ir_dgte: {
                        arith(vmi::dgte, vmi::dgtei, vmi::dlt);
                        break;
                    }
                    case op::ir_dneq: {
                        arith(vmi::dncmp, vmi::dncmpi, vmi::dncmpi);
                        break;
                    }
                    case op::ir_deq: {
                        arith(vmi::dcmp, vmi::dcmpi, vmi::dcmpi);
                        break;
                    }
                    case op::ir_reserve: break;
                    case op::ir_resolve:
                    case op::ir_assign: {
                        vmi rr, ri;
                        if (o1.isFloatingPoint() != o2.isFloatingPoint()) {
                            if (o2.isImm()) {
                                if (o1.isFloatingPoint()) {
                                    if (t1->getInfo().size == sizeof(f64)) {
                                        if (t2->getInfo().is_unsigned) m_instructions.push(encode(vmi::daddi).operand(r1).operand(vmr::zero).operand((f64)o2.getImm<u64>()));
                                        else m_instructions.push(encode(vmi::daddi).operand(r1).operand(vmr::zero).operand((f64)o2.getImm<i64>()));
                                    } else {
                                        if (t2->getInfo().is_unsigned) m_instructions.push(encode(vmi::faddi).operand(r1).operand(vmr::zero).operand((f32)o2.getImm<u64>()));
                                        else m_instructions.push(encode(vmi::faddi).operand(r1).operand(vmr::zero).operand((f32)o2.getImm<i64>()));
                                    }
                                } else {
                                    if (t1->getInfo().is_unsigned) {
                                        if (t2->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::addui).operand(r1).operand(vmr::zero).operand((u64)o2.getImm<f64>()));
                                        else m_instructions.push(encode(vmi::addui).operand(r1).operand(vmr::zero).operand((u64)o2.getImm<f32>()));
                                    } else {
                                        if (t2->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::addi).operand(r1).operand(vmr::zero).operand((i64)o2.getImm<f64>()));
                                        else m_instructions.push(encode(vmi::addi).operand(r1).operand(vmr::zero).operand((i64)o2.getImm<f32>()));
                                    }
                                }
                            } else {
                                if (o1.isFloatingPoint()) m_instructions.push(encode(vmi::mtfp).operand(r2).operand(r1));
                                else m_instructions.push(encode(vmi::mffp).operand(r2).operand(r1));
                            }
                            
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            break;
                        }

                        if (o1.isFloatingPoint()) {
                            if (t1->getInfo().size == sizeof(f64)) { rr = vmi::dadd; ri = vmi::daddi; }
                            else { rr = vmi::fadd; ri = vmi::faddi; }
                        } else {
                            if (t1->getInfo().is_unsigned || !t1->getInfo().is_primitive) { rr = vmi::addu; ri = vmi::addui; }
                            else { rr = vmi::add; ri = vmi::addi; }
                        }

                        if (o2.isImm()) {
                            if (t2->getInfo().is_floating_point) {
                                if (t2->getInfo().size == sizeof(f64)) m_instructions.push(encode(ri).operand(r1).operand(vmr::zero).operand(o2.getImm<f64>()));
                                else m_instructions.push(encode(ri).operand(r1).operand(vmr::zero).operand(o2.getImm<f32>()));
                            } else {
                                if (t2->getInfo().is_unsigned) m_instructions.push(encode(ri).operand(r1).operand(vmr::zero).operand(o2.getImm<u64>()));
                                else m_instructions.push(encode(ri).operand(r1).operand(vmr::zero).operand(o2.getImm<i64>()));
                            }
                        }
                        else m_instructions.push(encode(rr).operand(r1).operand(r2).operand(vmr::zero));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_ineg: {
                        m_instructions.push(encode(vmi::neg).operand(r1).operand(r2));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_fneg: {
                        m_instructions.push(encode(vmi::negf).operand(r1).operand(r2));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_dneg: {
                        m_instructions.push(encode(vmi::negd).operand(r1).operand(r2));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_call: {
                        FunctionType* sig = (FunctionType*)i.operands[0].getType();
                        
                        struct bk { vmr reg; u64 addr; u64 sz; };
                        std::vector<bk> backup;
                        robin_hood::unordered_map<vm_register, size_t> backup_map;
                        const auto& cfArgs = cf->getSignature()->getArguments();
                        
                        utils::Array<arg_location> calleeArgLocs;
                        getArgRegisters(sig, calleeArgLocs);

                        // backup caller's args
                        for (u8 a = 0;a < argLocs.size();a++) {
                            vmr ar = argLocs[a].reg_id;
                            if (ar == vmr::sp) continue;

                            auto alt = arg_lifetime_ends.find(ar);
                            if (alt == arg_lifetime_ends.end()) {
                                // arg is totally unused
                                continue;
                            }
                            if (alt->second < c) {
                                // arg is already unused
                                continue;
                            }

                            bool will_be_overwritten = false;
                            for (u8 a1 = 0;a1 < calleeArgLocs.size() && !will_be_overwritten;a1++) {
                                will_be_overwritten = calleeArgLocs[a1].reg_id == ar;
                            }

                            if (!will_be_overwritten) continue;
                            DataType* tp = cfArgs[a].dataType;

                            bool isPointer = false;
                            switch (cfArgs[a].argType) {
                                case arg_type::context_ptr:
                                case arg_type::pointer: {
                                    isPointer = true;
                                    break;
                                }
                                default: break;
                            }

                            u8 sz = tp->getInfo().size;
                            if (isPointer || !tp->getInfo().is_primitive) sz = sizeof(void*);
                            vmi st = vmi::st8;
                            switch (sz) {
                                case 2: { st = vmi::st16; break; }
                                case 4: { st = vmi::st32; break; }
                                case 8: { st = vmi::st64; break; }
                                default: { break; }
                            }
                            u64 sl = stack.alloc(sz);

                            backup.push_back({ ar, sl, sz });
                            backup_map[ar] = backup.size() - 1;

                            m_instructions.push(encode(st).operand(ar).operand(vmr::sp).operand(sl));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        }

                        // pass parameters
                        std::vector<u8> passed;
                        while (passed.size() < params.size()) {
                            for (u8 p = 0;p < params.size();p++) {
                                bool was_passed = false;
                                for (u8 p1 = 0;p1 < passed.size() && !was_passed;p1++) was_passed = passed[p1] == p;
                                if (was_passed) continue;
                                auto& calleeArgLoc = calleeArgLocs[p];

                                // determine if passing the parameter will overwrite another unpassed parameter
                                bool will_overwrite = false;
                                
                                // If the callee takes this arg on the stack, passing it won't overwrite any other parameters
                                if (calleeArgLoc.reg_id != vmr::sp) {
                                    for (u8 p1 = 0;p1 < params.size() && !will_overwrite;p1++) {
                                        if (p1 == p) continue;
                                        bool was_passed = false;
                                        for (u8 p2 = 0;p2 < passed.size() && !was_passed;p2++) was_passed = passed[p2] == p1;
                                        if (was_passed || params[p1].isStack() || params[p1].isImm()) continue;

                                        vmr reg;
                                        if (params[p1].isArg()) {
                                            reg = argLocs[params[p1].getImm<u32>()].reg_id;
                                            if (reg == vmr::sp) continue;
                                        } else {
                                            if (params[p1].isFloatingPoint()) reg = vmr(u32(vmr::f0) + (params[p1].getRegId() - 1));
                                            else reg = vmr(u32(vmr::s0) + (params[p1].getRegId() - 1));
                                        }
                                        will_overwrite = calleeArgLoc.reg_id == reg;
                                    }
                                }

                                if (will_overwrite) continue;
                                passed.push_back(p);

                                // pass the parameter
                                DataType* tp = params[p].getType();
                                vmr destReg = calleeArgLoc.reg_id;
                                if (destReg == vmr::sp) {
                                    if (params[p].isFloatingPoint()) destReg = vmr::vf3;
                                    else destReg = vmr::v3;
                                }

                                if (params[p].isImm()) {
                                    if (params[p].isFloatingPoint()) {
                                        if (tp->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::daddi).operand(destReg).operand(vmr::zero).operand(params[p].getImm<f64>()));
                                        else m_instructions.push(encode(vmi::faddi).operand(destReg).operand(vmr::zero).operand(params[p].getImm<f32>()));
                                    } else {
                                        m_instructions.push(encode(vmi::addui).operand(destReg).operand(vmr::zero).operand(params[p].getImm<u64>()));
                                    }
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else if (params[p].isStack()) {
                                    vmi ld = vmi::ld8;
                                    u64 sz = tp->getInfo().is_primitive ? tp->getInfo().size : sizeof(void*);
                                    switch (sz) {
                                        case 2: { ld = vmi::ld16; break; }
                                        case 4: { ld = vmi::ld32; break; }
                                        case 8: { ld = vmi::ld64; break; }
                                        default: { break; }
                                    }

                                    m_instructions.push(encode(ld).operand(destReg).operand(vmr::sp).operand(stack_addrs[params[p].getStackAllocId()]));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else {
                                    if (params[p].isArg()) {
                                        auto& loc = argLocs[params[p].getImm<u32>()];
                                        vmr reg = loc.reg_id;
                                        if (reg == vmr::sp) {
                                            vmi ld = vmi::ld8;
                                            switch (loc.stack_size) {
                                                case 2: { ld = vmi::ld16; break; }
                                                case 4: { ld = vmi::ld32; break; }
                                                case 8: { ld = vmi::ld64; break; }
                                                default: { break; }
                                            }

                                            m_instructions.push(encode(ld).operand(destReg).operand(vmr::sp).operand((u64)loc.stack_offset));
                                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                        } else {
                                            auto l = backup_map.find(reg);
                                            if (l != backup_map.end()) {
                                                // argument register was backed up
                                                bk& info = backup[l->second];

                                                vmi ld = vmi::ld8;
                                                switch (info.sz) {
                                                    case 2: { ld = vmi::ld16; break; }
                                                    case 4: { ld = vmi::ld32; break; }
                                                    case 8: { ld = vmi::ld64; break; }
                                                    default: { break; }
                                                }

                                                m_instructions.push(encode(ld).operand(destReg).operand(vmr::sp).operand((u64)info.addr));
                                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                            } else {
                                                if (params[p].isFloatingPoint()) {
                                                    if (tp->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::dadd).operand(destReg).operand(vmr::zero).operand(reg));
                                                    else m_instructions.push(encode(vmi::fadd).operand(destReg).operand(vmr::zero).operand(reg));
                                                } else {
                                                    m_instructions.push(encode(vmi::addu).operand(destReg).operand(vmr::zero).operand(reg));
                                                }

                                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                            }
                                        }
                                    } else {
                                        vmr reg;

                                        if (params[p].isFloatingPoint()) reg = vmr(u32(vmr::f0) + (params[p].getRegId() - 1));
                                        else reg = vmr(u32(vmr::s0) + (params[p].getRegId() - 1));

                                        if (params[p].isFloatingPoint()) {
                                            if (tp->getInfo().size == sizeof(f64)) m_instructions.push(encode(vmi::dadd).operand(destReg).operand(vmr::zero).operand(reg));
                                            else m_instructions.push(encode(vmi::fadd).operand(destReg).operand(vmr::zero).operand(reg));
                                        } else {
                                            m_instructions.push(encode(vmi::addu).operand(destReg).operand(vmr::zero).operand(reg));
                                        }

                                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                    }
                                }

                                if (calleeArgLoc.reg_id == vmr::sp) {
                                    // value of destReg must be moved onto the stack... in the callee's stack frame...
                                    // sp hasn't been offset yet, so the offset must be calculated relative to where
                                    // the new stack pointer will be
                                    
                                    vmi st = vmi::st8;
                                    switch (calleeArgLoc.stack_size) {
                                        case 2: { st = vmi::st16; break; }
                                        case 4: { st = vmi::st32; break; }
                                        case 8: { st = vmi::st64; break; }
                                        default: { break; }
                                    }

                                    u64 offset = calleeArgLoc.stack_offset + stack.size();

                                    m_instructions.push(encode(st).operand(destReg).operand(vmr::sp).operand(offset));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                }
                            }
                        }

                        params.clear();

                        // do the call
                        if (i.operands[0].isImm()) {
                            // offset $sp
                            m_instructions.push(encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand((u64)stack.size()));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());

                            m_instructions.push(encode(vmi::jal).operand((u64)i.operands[0].getImm<function_id>()));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        } else {
                            vmr cr = r1;
                            auto l = backup_map.find(cr);
                            if (l != backup_map.end()) {
                                cr = vmr::v0;
                                // callee register was backed up
                                bk& info = backup[l->second];

                                m_instructions.push(encode(vmi::ld64).operand(cr).operand(vmr::sp).operand((u64)info.addr));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }

                            // offset $sp
                            m_instructions.push(encode(vmi::addui).operand(vmr::sp).operand(vmr::sp).operand((u64)stack.size()));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());

                            // jalr
                            m_instructions.push(encode(vmi::jalr).operand(cr));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        }

                        // restore $sp
                        m_instructions.push(encode(vmi::subui).operand(vmr::sp).operand(vmr::sp).operand((u64)stack.size()));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());

                        // restore backed up registers
                        for (u16 b = 0;b < backup.size();b++) {
                            vmi ld = vmi::ld8;
                            switch (backup[b].sz) {
                                case 2: { ld = vmi::ld16; break; }
                                case 4: { ld = vmi::ld32; break; }
                                case 8: { ld = vmi::ld64; break; }
                                default: { break; }
                            }

                            m_instructions.push(encode(ld).operand(backup[b].reg).operand(vmr::sp).operand(backup[b].addr));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            stack.free((u32)backup[b].addr);
                        }

                        break;
                    }
                    case op::ir_param: {
                        params.push(o1);
                        break;
                    }
                    case op::ir_ret: {
                        m_instructions.push(encode(vmi::jmpr).operand(vmr::ra));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                    case op::ir_cvt: {
                        DataType* from = o2.getType();
                        DataType* to = m_ctx->getTypes()->getType(o3.getImm<type_id>());

                        if (o2.isImm()) {
                            vmi ci = vmi::instruction_count;
                            if (to->getInfo().is_floating_point) {
                                if (to->getInfo().size == sizeof(f32)) ci = vmi::faddi;
                                else ci = vmi::daddi;
                            } else {
                                if (to->getInfo().is_unsigned) ci = vmi::addui;
                                else ci = vmi::addi;
                            }
                            auto instr = encode(ci).operand(r1).operand(vmr::zero);

                            if (from->getInfo().is_floating_point) {
                                if (to->getInfo().is_floating_point) {
                                    if (to->getInfo().size == sizeof(f32)) instr.operand((f32)o2.getImm<f64>());
                                    else instr.operand(o2.getImm<f64>());
                                } else {
                                    if (to->getInfo().is_unsigned) {
                                        u64 imm = 0;
                                        switch (to->getInfo().size) {
                                            case 1: { *((u8 *)&imm) = (u8 )o2.getImm<f64>(); break; }
                                            case 2: { *((u16*)&imm) = (u16)o2.getImm<f64>(); break; }
                                            case 4: { *((u32*)&imm) = (u32)o2.getImm<f64>(); break; }
                                            case 8: { *((u64*)&imm) = (u64)o2.getImm<f64>(); break; }
                                        }
                                        instr.operand(imm);
                                    } else {
                                        i64 imm = 0;
                                        switch (to->getInfo().size) {
                                            case 1: { *((i8 *)&imm) = (i8 )o2.getImm<f64>(); break; }
                                            case 2: { *((i16*)&imm) = (i16)o2.getImm<f64>(); break; }
                                            case 4: { *((i32*)&imm) = (i32)o2.getImm<f64>(); break; }
                                            case 8: { *((i64*)&imm) = (i64)o2.getImm<f64>(); break; }
                                        }
                                        instr.operand(imm);
                                    }
                                }
                            } else {
                                if (from->getInfo().is_unsigned) {
                                    if (to->getInfo().is_floating_point) {
                                        if (to->getInfo().size == sizeof(f32)) instr.operand((f32)o2.getImm<u64>());
                                        else instr.operand((f64)o2.getImm<u64>());
                                    } else {
                                        if (to->getInfo().is_unsigned) {
                                            u64 imm = 0;
                                            switch (to->getInfo().size) {
                                                case 1: { *((u8 *)&imm) = (u8 )o2.getImm<u64>(); break; }
                                                case 2: { *((u16*)&imm) = (u16)o2.getImm<u64>(); break; }
                                                case 4: { *((u32*)&imm) = (u32)o2.getImm<u64>(); break; }
                                                case 8: { *((u64*)&imm) = (u64)o2.getImm<u64>(); break; }
                                            }
                                            instr.operand(imm);
                                        } else {
                                            i64 imm = 0;
                                            switch (to->getInfo().size) {
                                                case 1: { *((i8 *)&imm) = (i8 )o2.getImm<u64>(); break; }
                                                case 2: { *((i16*)&imm) = (i16)o2.getImm<u64>(); break; }
                                                case 4: { *((i32*)&imm) = (i32)o2.getImm<u64>(); break; }
                                                case 8: { *((i64*)&imm) = (i64)o2.getImm<u64>(); break; }
                                            }
                                            instr.operand(imm);
                                        }
                                    }
                                } else {
                                    if (to->getInfo().is_floating_point) {
                                        if (to->getInfo().size == sizeof(f32)) instr.operand((f32)o2.getImm<i64>());
                                        else instr.operand((f64)o2.getImm<i64>());
                                    } else {
                                        if (to->getInfo().is_unsigned) {
                                            u64 imm = 0;
                                            switch (to->getInfo().size) {
                                                case 1: { *((u8 *)&imm) = (u8 )o2.getImm<i64>(); break; }
                                                case 2: { *((u16*)&imm) = (u16)o2.getImm<i64>(); break; }
                                                case 4: { *((u32*)&imm) = (u32)o2.getImm<i64>(); break; }
                                                case 8: { *((u64*)&imm) = (u64)o2.getImm<i64>(); break; }
                                            }
                                            instr.operand(imm);
                                        } else {
                                            i64 imm = 0;
                                            switch (to->getInfo().size) {
                                                case 1: { *((i8 *)&imm) = (i8 )o2.getImm<i64>(); break; }
                                                case 2: { *((i16*)&imm) = (i16)o2.getImm<i64>(); break; }
                                                case 4: { *((i32*)&imm) = (i32)o2.getImm<i64>(); break; }
                                                case 8: { *((i64*)&imm) = (i64)o2.getImm<i64>(); break; }
                                            }
                                            instr.operand(imm);
                                        }
                                    }
                                }
                            }

                            m_instructions.push(instr);
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            break;
                        }

                        vmi ci = vmi::instruction_count;

                        if (from->getInfo().is_floating_point) {
                            if (from->getInfo().size == sizeof(f64)) {
                                if (to->getInfo().is_floating_point) {
                                    if (to->getInfo().size == sizeof(f32)) ci = vmi::cvt_df;

                                    m_instructions.push(encode(vmi::dadd).operand(r1).operand(r2).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else {
                                    if (to->getInfo().is_unsigned) ci = vmi::cvt_du;
                                    else ci = vmi::cvt_di;

                                    m_instructions.push(encode(vmi::mffp).operand(r2).operand(r1));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                }
                            } else {
                                if (to->getInfo().is_floating_point) {
                                    if (to->getInfo().size == sizeof(f64)) ci = vmi::cvt_fd;
                                    
                                    m_instructions.push(encode(vmi::fadd).operand(r1).operand(r2).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else {
                                    if (to->getInfo().is_unsigned) ci = vmi::cvt_fu;
                                    else ci = vmi::cvt_fi;

                                    m_instructions.push(encode(vmi::mffp).operand(r2).operand(r1));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                }
                            }
                        } else {
                            if (from->getInfo().is_unsigned) {
                                if (to->getInfo().is_floating_point) {
                                    if (to->getInfo().size == sizeof(f64)) ci = vmi::cvt_ud;
                                    else ci = vmi::cvt_uf;
                                    
                                    m_instructions.push(encode(vmi::mtfp).operand(r2).operand(r1).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else {
                                    if (!to->getInfo().is_unsigned) ci = vmi::cvt_ui;

                                    m_instructions.push(encode(vmi::addu).operand(r1).operand(r2).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                }
                            } else {
                                if (to->getInfo().is_floating_point) {
                                    if (to->getInfo().size == sizeof(f64)) ci = vmi::cvt_id;
                                    else ci = vmi::cvt_if;
                                    
                                    m_instructions.push(encode(vmi::mtfp).operand(r2).operand(r1).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                } else {
                                    if (to->getInfo().is_unsigned) ci = vmi::cvt_iu;

                                    m_instructions.push(encode(vmi::add).operand(r1).operand(r2).operand(vmr::zero));
                                    m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                                }
                            }
                        }

                        if (ci != vmi::instruction_count) {
                            m_instructions.push(encode(ci).operand(r1));
                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        }

                        break;
                    }
                    case op::ir_label: {
                        label_map[o1.getImm<label_id>()] = m_instructions.size();
                        break;
                    }
                    case op::ir_branch: {
                        label_id truthLabel = o2.getImm<label_id>();
                        label_id falthLabel = o3.getImm<label_id>();

                        // todo: Why did I add the b* branch instructions if only bneqz is used

                        if (is_fpr(r1)) {
                            if (t1->getInfo().size == sizeof(f64)) {
                                m_instructions.push(encode(vmi::dncmp).operand(vmr::v1).operand(r1).operand(vmr::zero));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            } else {
                                m_instructions.push(encode(vmi::fncmp).operand(vmr::v1).operand(r1).operand(vmr::zero));
                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }

                            auto lb = label_map.find(falthLabel);
                            if (lb != label_map.end()) m_instructions.push(encode(vmi::bneqz).operand(vmr::v1).operand((u64)lb->second));
                            else {
                                deferred_label_instrs.push({ m_instructions.size(), falthLabel });
                                m_instructions.push(encode(vmi::bneqz).operand(vmr::v1));
                            }

                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());

                            address taddr = ch->labels.get(truthLabel);
                            if (taddr != c + 1) {
                                // branch truth label is not immediately after the branch, must jump to it
                                auto tlb = label_map.find(truthLabel);
                                if (tlb != label_map.end()) m_instructions.push(encode(vmi::jmp).operand((u64)tlb->second));
                                else {
                                    deferred_label_instrs.push({ m_instructions.size(), truthLabel });
                                    m_instructions.push(encode(vmi::bneqz));
                                }

                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        } else {
                            auto lb = label_map.find(falthLabel);
                            if (lb != label_map.end()) m_instructions.push(encode(vmi::bneqz).operand(r1).operand((u64)lb->second));
                            else {
                                deferred_label_instrs.push({ m_instructions.size(), falthLabel });
                                m_instructions.push(encode(vmi::bneqz).operand(r1));
                            }

                            m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());

                            address taddr = ch->labels.get(truthLabel);
                            if (taddr != c + 1) {
                                // branch truth label is not immediately after the branch, must jump to it
                                auto tlb = label_map.find(truthLabel);
                                if (tlb != label_map.end()) m_instructions.push(encode(vmi::jmp).operand((u64)tlb->second));
                                else {
                                    deferred_label_instrs.push({ m_instructions.size(), truthLabel });
                                    m_instructions.push(encode(vmi::bneqz));
                                }

                                m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                            }
                        }
                        break;
                    }
                    case op::ir_jump: {
                        label_id lbl = o1.getImm<label_id>();

                        auto lb = label_map.find(lbl);
                        if (lb != label_map.end()) m_instructions.push(encode(vmi::jmp).operand((u64)lb->second));
                        else {
                            deferred_label_instrs.push({ m_instructions.size(), lbl });
                            m_instructions.push(encode(vmi::jmp));
                        }

                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                        break;
                    }
                }

                
                if (iinfo.assigns_operand_index != 0xFF) {
                    const var* assigned = nullptr;
                    vm_register assignedReg = vmr::register_count;
                    switch (iinfo.assigns_operand_index) {
                        case 0: {
                            assigned = &i.operands[0];
                            assignedReg = r1;
                            break;
                        }
                        case 1: {
                            assigned = &i.operands[1];
                            assignedReg = r2;
                            break;
                        }
                        case 2: {
                            assigned = &i.operands[2];
                            assignedReg = r3;
                            break;
                        }
                    }

                    if (assigned->isStack() || (assigned->isArg() && argLocs[assigned->getImm<u32>()].reg_id == vmr::sp)) {
                        u8 sz = assigned->getType()->getInfo().size;
                        if (!assigned->getType()->getInfo().is_primitive) sz = sizeof(void*);
                        vmi st = vmi::st8;
                        switch (sz) {
                            case 2: { st = vmi::st16; break; }
                            case 4: { st = vmi::st32; break; }
                            case 8: { st = vmi::st64; break; }
                            default: { break; }
                        }

                        u64 stackOffset = 0;
                        if (assigned->isArg()) stackOffset = (u64)argLocs[assigned->getImm<u32>()].stack_offset;
                        else stackOffset = stack_addrs[assigned->getStackAllocId()];

                        m_instructions.push(encode(st).operand(assignedReg).operand(vmr::sp).operand(stackOffset));
                        m_map.add(i.src.getLine(), i.src.getCol(), i.src.getLength());
                    }
                }
            }

            // update jumps and branches which referred to labels that were not
            // processed yet at the time of their generation
            for (u32 i = 0;i < deferred_label_instrs.size();i++) {
                vm::Instruction& instr = m_instructions[deferred_label_instrs[i].idx];
                auto it = label_map.find(deferred_label_instrs[i].label);
                if (it == label_map.end()) {
                    // todo runtime error
                    break;
                }

                address addr = it->second;
                instr.operand((u64)addr);
            }

            // all functions should end with jmpr $ra, that final instruction
            // will be removed and replaced with the epilogue, and jmpr $ra will
            // be added back afterward
            address epilog_addr = m_instructions.size() - 1;
            m_instructions.remove(epilog_addr);
            m_map.remove(epilog_addr);

            // replace all jmpr $ra instructions with jmp epilog_addr
            for (address c = 0;c < m_instructions.size();c++) {
                auto& i = m_instructions[c];
                if (i.instr() != vmi::jmp || i.op_1r() != vmr::ra) continue;
                m_instructions[c] = encode(vmi::jmp).operand((u64)epilog_addr);
            }

            const auto& src = cf->getSource();

            // generate function epilogue
            for (auto& x : prologue) {
                m_instructions.push(encode(vmi::ld64).operand(x.reg).operand(vmr::sp).operand((u64)x.addr));
                m_map.add(src.getLine(), src.getCol(), src.getLength());
            }

            // add jmpr $ra
            m_instructions.push(encode(vmi::jmpr).operand(vmr::ra));
            m_map.add(src.getLine(), src.getCol(), src.getLength());

            addFunction(cf);
            m_funcData[cf] = {
                out_begin,
                m_instructions.size()
            };
        }

        void Backend::call(ffi::Function* func, call_context* ctx, void* retPtr, void** args) {
            const function_data* fd = getFunctionData(func);
            if (!fd) return; // todo exception

            // todo: Move this to function data map
            utils::Array<arg_location> argLocs;
            getArgRegisters(func->getSignature(), argLocs);

            const auto& argInfo = func->getSignature()->getArguments();

            VM* vm = m_vms->get();
            vm->prepareState();

            u32 explicitIdx = 0;
            for (u32 a = 0;a < argInfo.size();a++) {
                arg_location& loc = argLocs[a];
                void* dest = nullptr;
                if (loc.reg_id == vmr::sp) dest = (void*)(vm->state.registers[(u8)vmr::sp] + loc.stack_offset);
                else dest = &vm->state.registers[(u8)loc.reg_id];

                if (argInfo[a].argType == arg_type::context_ptr) *((call_context**)dest) = ctx;
                else {
                    *((void**)dest) = args[explicitIdx++];
                }
            }

            vm->execute(m_instructions, fd->begin);
        }

        VM* Backend::getVM() {
            return m_vms->get();
        }

        const utils::Array<Instruction>& Backend::getCode() const {
            return m_instructions;
        }
        
        const function_data* Backend::getFunctionData(ffi::Function* func) const {
            auto it = m_funcData.find(func);
            if (it == m_funcData.end()) return nullptr;

            return &it->second;
        }
        
        const SourceMap& Backend::getSourceMap() const {
            return m_map;
        }
    };
};