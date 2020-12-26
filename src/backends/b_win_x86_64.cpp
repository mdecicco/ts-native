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
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace orc;

#ifdef _MSC_VER
extern "C" {
    // not sure why this isn't linked automatically by LLVM
    void __chkstk();
};
#endif

namespace gjs {
    // forward declarations
    Value* find_value (w64_gen_ctx& ctx, u32 block, u64 regId, robin_hood::unordered_node_set<u32>& checked);

    void block_compile_order (w64_gen_ctx& ctx, u32 blockId, robin_hood::unordered_node_set<u32>& added, std::vector<u32>& out);


    // structs
    struct reg_usage_info {
        u64 first_non_assignment_use_addr;
        u64 first_assignment_addr;

        bool unused;
        bool assigned;
    };

    struct pending_phi {
        u64 regId;
        PHINode* phi;
        BasicBlock* when_from;
    };

    struct ir_block {
        BasicBlock* block;
        robin_hood::unordered_map<u64, llvm::Value*> var_map;
        std::vector<pending_phi> pending_phis;
        bool compiled;
        bool fallthrough;
        u64 begin;
        u64 end;
    };

    struct ir_block_link {
        u64 from;
        u64 to;
    };

    struct w64_gen_ctx {
        llvm::LLVMContext* ctx;
        llvm::Module* mod;
        llvm::Function* target;
        llvm::IRBuilder<>* builder;
        compilation_output& input;
        u32 cur_function_idx;
        u32 cur_block_idx;
        u64 cur_instr_idx;
        std::vector<ir_block> blocks;
        std::vector<ir_block_link> links;
        std::vector<llvm::Value*> call_params;

        // look up value of a virtual register
        // traverses the block graph upwards starting from
        // cur_block_idx to find one that has a value for the register
        // returns null if it's not found
        Value* get(u64 regId) {
            robin_hood::unordered_node_set<u32> checked;
            return find_value(*this, cur_block_idx, regId, checked);
        }

        // 'assign' a value to a virtual register
        void set(u64 regId, Value* v) {
            blocks[cur_block_idx].var_map[regId] = v;
        }

        // get info about a block's usage of a register
        reg_usage_info reg_usage(u32 blockId, u64 regId) {
            reg_usage_info out = { 0, 0, true, false };

            bool found_non_assign_use = false;

            for (u64 c = blocks[blockId].begin;c <= blocks[blockId].end;c++) {
                auto& i = input.code[c];

                auto& v0 = i.operands[0];
                auto& v1 = i.operands[1];
                auto& v2 = i.operands[2];

                bool used = v0.valid() && v0.reg_id() == regId;
                if (!used) used = v1.valid() && v1.reg_id() == regId;
                if (!used) used = v2.valid() && v2.reg_id() == regId;

                if (used) {
                    if (compile::is_assignment(i)) {
                        bool assigns_regId = v0.reg_id() == regId;
                        if (assigns_regId && !out.assigned) {
                            out.first_assignment_addr = c;
                            out.assigned = true;
                            out.unused = false;
                        } else if (!assigns_regId && !found_non_assign_use) {
                            out.first_non_assignment_use_addr = c;
                            out.unused = false;
                            found_non_assign_use = true;
                        }
                    } else if (!found_non_assign_use) {
                        out.first_non_assignment_use_addr = c;
                        out.unused = false;
                        found_non_assign_use = true;
                    }
                }

                if (found_non_assign_use && out.assigned) {
                    // nothing left to do
                    return out;
                }
            }

            return out;
        }

        // get list of registers that may need to have
        // phi nodes generated within the given block
        u32 potential_phi_regs(u32 blockId, std::vector<const compile::var*>& regIds) {
            robin_hood::unordered_node_set<u64> assigned;
            robin_hood::unordered_node_set<u64> in_output;

            // Determine registers that are used before being assigned
            // (if a register is assigned before being used, then the
            // block doesn't need to know the value from any previous
            // assignments)
            for (u64 c = blocks[blockId].begin;c <= blocks[blockId].end;c++) {
                auto& i = input.code[c];

                auto& v0 = i.operands[0];
                auto& v1 = i.operands[1];
                auto& v2 = i.operands[2];

                if (compile::is_assignment(i)) {
                    assigned.insert(v0.reg_id());
                    if (i.op == compile::operation::eq && v1.reg_id() == v0.reg_id() && in_output.count(v1.reg_id()) == 0) {
                        in_output.insert(v1.reg_id());
                        regIds.push_back(&v1);
                    }
                } else {
                    if (v0.valid() && v0.reg_id() != u32(-1) && assigned.count(v0.reg_id()) == 0 && in_output.count(v0.reg_id()) == 0) {
                        in_output.insert(v0.reg_id());
                        regIds.push_back(&v0);
                    }
                }

                if (v1.valid() && v1.reg_id() != u32(-1) && assigned.count(v1.reg_id()) == 0 && in_output.count(v1.reg_id()) == 0) {
                    in_output.insert(v1.reg_id());
                    regIds.push_back(&v1);
                }

                if (v2.valid() && v2.reg_id() != u32(-1) && assigned.count(v2.reg_id()) == 0 && in_output.count(v2.reg_id()) == 0) {
                    in_output.insert(v2.reg_id());
                    regIds.push_back(&v2);
                }
            }

            // Determine registers that are assigned in
            // subsequent blocks that link back to blockId
            for (u32 sb = 0;sb < blocks.size();sb++) {
                if (is_linked(blockId, sb) && is_linked(sb, blockId)) {
                    for (u64 c = blocks[sb].begin;c <= blocks[sb].end;c++) {
                        auto& i = input.code[c];

                        auto& v0 = i.operands[0];

                        if (compile::is_assignment(i)) {
                            if (in_output.count(v0.reg_id()) == 0) {
                                in_output.insert(v0.reg_id());
                                regIds.push_back(&v0);
                            }
                            continue;
                        }
                    }
                }
            }

            return regIds.size();
        }

        // returns true if block 'from' should flow into block 'to'
        bool is_linked(u32 from, u32 to) {
            for (u32 l = 0;l < links.size();l++) {
                if (links[l].from == from && links[l].to == to) return true;
            }
            return false;
        }

        // get the order which blocks should (probably) be compiled in
        void block_order (std::vector<u32>& order) {
            robin_hood::unordered_node_set<u32> added;
            block_compile_order(*this, 0, added, order);
        }
    };

    struct phi_def {
        // type of value
        Type* valueType;
        // blockId to select the value from
        u32 value_source;
        // select the value when control passes from this blockId
        u32 when_from;
    };


    // codegen helper functions
    Type* llvm_type(script_type* tp, w64_gen_ctx& g) {
        if (tp->is_primitive) {
            if (tp->is_floating_point) {
                if (tp->size == sizeof(f64)) return Type::getDoubleTy(*g.ctx);
                else return Type::getFloatTy(*g.ctx);
            } else {
                switch (tp->size) {
                    case 0: return Type::getVoidTy(*g.ctx);
                    case 1: {
                        if (tp->name == "bool") return Type::getInt1Ty(*g.ctx);
                        return Type::getInt8Ty(*g.ctx);
                    }
                    case 2: return Type::getInt16Ty(*g.ctx);
                    case 4: return Type::getInt32Ty(*g.ctx);
                    case 8: return Type::getInt64Ty(*g.ctx);
                }
            }
        } else {
            return Type::getInt64Ty(*g.ctx); // structs are always pointers in gjs ir
            /*
            u64 id = join_u32(tp->owner->id(), tp->id());
            auto it = types.find(id);
            if (it == types.end()) return nullptr;
            return it->second;
            */
        }

        return nullptr;
    }

    Value* llvm_value(const compile::var& v, w64_gen_ctx& g) {
        if (v.is_imm()) {
            if (v.type()->is_floating_point) {
                if (v.type()->size == sizeof(f64)) {
                    return ConstantFP::get(llvm_type(v.type(), g), (double)v.imm_d());
                } else {
                    return ConstantFP::get(llvm_type(v.type(), g), (float)v.imm_f());
                }
            } else {
                return ConstantInt::get(llvm_type(v.type(), g), v.imm_u(), !v.type()->is_unsigned);
            }
        } else if (v.is_arg()) {
            return g.target->getArg(v.arg_idx());
        } else {
            return g.get(v.reg_id());
        }

        return nullptr;
    }


    // code flow graph helper functions
    void block_compile_order (w64_gen_ctx& ctx, u32 blockId, robin_hood::unordered_node_set<u32>& added, std::vector<u32>& out) {
        if (added.count(blockId) == 0) {
            added.insert(blockId);
            out.push_back(blockId);
        }

        for (u32 i = 0;i < ctx.links.size();i++) {
            if (ctx.links[i].from == blockId && added.count(ctx.links[i].to) == 0) {
                block_compile_order(ctx, ctx.links[i].to, added, out);
            }
        }
    }

    Value* find_value(w64_gen_ctx& ctx, u32 blockId, u64 regId, robin_hood::unordered_node_set<u32>& checked) {
        auto it = ctx.blocks[blockId].var_map.find(regId);
        if (it != ctx.blocks[blockId].var_map.end()) return it->second;

        checked.insert(blockId);

        for (u32 i = 0;i < ctx.links.size();i++) {
            if (ctx.links[i].to == blockId && !checked.count(ctx.links[i].from)) {
                Value* v = find_value(ctx, ctx.links[i].from, regId, checked);
                if (v) return v;
            }
        }

        return nullptr;
    }

    void add_link(const ir_block_link& l, std::vector<ir_block_link>& links) {
        for (u32 i = 0;i < links.size();i++) {
            if (links[i].from == l.from && links[i].to == l.to) return;
        }

        links.push_back(l);
    }

    void gen_blocks(compilation_output::ir_code& code, u64 start, u64 end, std::vector<ir_block>& blocks, std::vector<ir_block_link>& links) {
        using op = compile::operation;
        ir_block b = { nullptr, robin_hood::unordered_map<u64, Value*>(), { }, false, false, start, start };

        for (u64 c = start;c <= end;c++) {
            const compile::tac_instruction& i = code[c];
            b.end = c;

            if (i.op == op::meta_if_branch) {
                // end current block
                b.end++; // end on the branch instruction
                blocks.push_back(b);
                b.begin = b.end = c;

                // 
                u64 trueBlockEndAddr = i.operands[0].imm_u();
                u64 falseBlockEndAddr = i.operands[1].imm_u();
                u64 joinAddr = i.operands[2].imm_u();

                u32 parentBlock = blocks.size() - 1;
                u32 trueBlock = parentBlock + 1;
                u32 falseBlock = 0;

                // truth block starts at c + 2 because the next one is the branch instruction
                add_link({ parentBlock, trueBlock }, links);
                gen_blocks(code, c + 2, trueBlockEndAddr, blocks, links);

                if (falseBlockEndAddr) {
                    falseBlock = blocks.size();
                    add_link({ parentBlock, falseBlock }, links);
                    gen_blocks(code, trueBlockEndAddr + 1, falseBlockEndAddr, blocks, links);
                } else {
                    add_link({ parentBlock, blocks.size() }, links);
                }

                add_link({ trueBlock, blocks.size() }, links);
                if (falseBlock) add_link({ falseBlock, blocks.size() }, links);

                b.begin = b.end = c = joinAddr;
                c--;
            }
            else if (i.op == op::meta_for_loop) {
                if (b.end > start) {
                    b.end--;
                    b.fallthrough = true;
                    blocks.push_back(b);
                    b.begin = b.end = c;
                    b.fallthrough = false;
                }

                u64 branchAddr = i.operands[0].imm_u();
                u64 endAddr = i.operands[1].imm_u();
                u32 entryBlock = blocks.size();

                // cond/branch
                if (b.end > start) add_link({ blocks.size() - 1, entryBlock }, links); // parent -> cond/branch
                gen_blocks(code, c + 1, branchAddr, blocks, links);
                add_link({ entryBlock, blocks.size() }, links); // cond/branch -> body

                // body
                gen_blocks(code, branchAddr + 1, endAddr, blocks, links);
                add_link({ blocks.size() - 1, entryBlock }, links); // body -> cond/branch
                add_link({ entryBlock, blocks.size() }, links); // cond/branch -> post-loop

                b.begin = b.end = c = endAddr + 1;
                c--;
            }
            else if (i.op == op::meta_while_loop) {
                if (b.end > start) {
                    b.end--;
                    b.fallthrough = true;
                    blocks.push_back(b);
                    b.begin = b.end = c;
                    b.fallthrough = false;
                }

                u64 branchAddr = i.operands[0].imm_u();
                u64 endAddr = i.operands[1].imm_u();
                u32 entryBlock = blocks.size();

                // cond/branch
                if (b.end > start) add_link({ blocks.size() - 1, entryBlock }, links); // parent -> cond/branch
                gen_blocks(code, c + 1, branchAddr, blocks, links);
                add_link({ entryBlock, blocks.size() }, links); // cond/branch -> body

                // body
                gen_blocks(code, branchAddr + 1, endAddr, blocks, links);
                add_link({ blocks.size() - 1, entryBlock }, links); // body -> cond/branch
                add_link({ entryBlock, blocks.size() }, links); // cond/branch -> post-loop

                b.begin = b.end = c = endAddr + 1;
                c--;
            }
            else if (i.op == op::meta_do_while_loop) {
                if (b.end > start) {
                    b.end--;
                    b.fallthrough = true;
                    blocks.push_back(b);
                    b.begin = b.end = c;
                    b.fallthrough = false;
                }

                u64 branchAddr = i.operands[0].imm_u();
                u32 entryBlock = blocks.size();

                // body
                if (b.end > start) add_link({ blocks.size() - 1, entryBlock }, links); // parent -> body
                gen_blocks(code, c + 1, branchAddr, blocks, links);
                add_link({ blocks.size() - 1, blocks.size() }, links); // body -> jump
                add_link({ blocks.size() - 1, blocks.size() + 1 }, links); // body -> post-loop

                // jump
                gen_blocks(code, branchAddr + 1, branchAddr + 1, blocks, links);
                add_link({ blocks.size() - 1, entryBlock }, links); // jump -> body

                b.begin = b.end = c = branchAddr + 2;
                c--;
            }
            else if (i.op == op::ret || i.op == op::jump) {
                blocks.push_back(b);
                b.begin = b.end = c;
            }
        }

        if (b.end > b.begin) blocks.push_back(b);
    }

    u32 nearest_assigning(w64_gen_ctx& g, u32 blockId, u64 regId, robin_hood::unordered_node_set<u32>& checked) {
        if (g.blocks[blockId].var_map.count(regId) || g.reg_usage(blockId, regId).assigned) return blockId;
        else checked.insert(blockId);

        for (u32 b = 0;b < g.blocks.size();b++) {
            if (g.is_linked(b, blockId) && !checked.count(b)) {
                u32 result = nearest_assigning(g, b, regId, checked);
                if (result != u32(-1)) return result;
            }
        }

        return u32(-1);
    }

    robin_hood::unordered_map<u64, std::vector<phi_def>> gen_phis(w64_gen_ctx& g, u32 blockId) {
        std::vector<u32> parents;
        for (u32 b = 0;b < g.blocks.size();b++) {
            if (g.is_linked(b, blockId)) parents.push_back(b);
        }

        robin_hood::unordered_map<u64, std::vector<phi_def>> phis;
        if (parents.size() > 1) {
            // case 1:
            // multiple parents which assign a register that is used by blockId,
            // or have descendents that assign said register

            std::vector<const compile::var*> regs;
            g.potential_phi_regs(blockId, regs);

            // for each register/parent pair,
            // determine nearest parent block which assigns it
            for (u32 r = 0;r < regs.size();r++) {
                for (u32 p = 0;p < parents.size();p++) {
                    robin_hood::unordered_node_set<u32> checked;
                    u32 result = nearest_assigning(g, parents[p], regs[r]->reg_id(), checked);
                    if (result != u32(-1)) {
                        phis[regs[r]->reg_id()].push_back({
                            llvm_type(regs[r]->type(), g),
                            result,
                            parents[p]
                        });
                    }
                }

                if (phis[regs[r]->reg_id()].size() == 1) phis.erase(regs[r]->reg_id());
            }
        }

        return phis;
    }

    
    // class made just to keep LLVM includes out of the header file
    class llvm_data {
        public:
            llvm_data() {
                //m_llvm_init = new InitLLVM(argc, argv);

                InitializeNativeTarget();
                InitializeNativeTargetAsmPrinter();
                InitializeNativeTargetAsmParser();

                tsc = std::make_unique<LLVMContext>();
                auto jitb = LLJITBuilder().create();
                if (!jitb) {
                    // exception
                }

                jit = std::move(jitb.get());
            }

            ~llvm_data() {
                // delete m_llvm_init;
                // todo: figure out how to _actually_ release all of llvm's resources
            }

            LLVMContext* ctx() { return tsc.getContext(); }

            //InitLLVM* m_llvm_init;
            ThreadSafeContext tsc;
            std::unique_ptr<LLJIT> jit;
            robin_hood::unordered_map<std::string, ThreadSafeModule> modules;
            robin_hood::unordered_map<u64, Type*> types;
    };


    win64_backend::win64_backend() : m_log_ir(false), m_llvm(new llvm_data()) {
    }

    win64_backend::~win64_backend() {
        delete m_llvm;
    }

    void win64_backend::init() {
        auto& DL = m_llvm->jit->getDataLayout();
        auto& ES = m_llvm->jit->getExecutionSession();
        auto& lib = m_llvm->jit->getMainJITDylib();

        auto mdata = &bind::call_class_method<void*, script_buffer, void* (script_buffer::*)(u64), script_buffer*, u64>;
        SymbolMap symbols;
        symbols[ES.intern("__mdata")] = { pointerToJITTargetAddress(mdata), JITSymbolFlags::Exported | JITSymbolFlags::Absolute };
        
        #ifdef _MSC_VER
        symbols[ES.intern("__chkstk")] = { pointerToJITTargetAddress(&__chkstk), JITSymbolFlags::Exported | JITSymbolFlags::Absolute };
        #endif

        lib.define(absoluteSymbols(symbols));
    }

    void win64_backend::commit_bindings() {
        auto& DL = m_llvm->jit->getDataLayout();
        auto& ES = m_llvm->jit->getExecutionSession();
        auto& lib = m_llvm->jit->getMainJITDylib();

        SymbolMap symbols;
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
        std::unique_ptr<Module> mod = std::make_unique<Module>(in.mod->name(), *m_llvm->ctx());
        w64_gen_ctx g = { m_llvm->ctx(), mod.get(), nullptr, nullptr, in, 0, 0, 0, {}, {}, {} };

        add_bindings_to_module(g);

        // forward declare
        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;

            std::vector<Type*> ptypes;
            for (u8 a = 0;a < in.funcs[f].func->signature.arg_types.size();a++) {
                ptypes.push_back(llvm_type(in.funcs[f].func->signature.arg_types[a], g));
            }

            Function* func = Function::Create(
                FunctionType::get(llvm_type(in.funcs[f].func->signature.return_type, g), ptypes, false),
                Function::ExternalLinkage,
                internal_func_name(in.funcs[f].func),
                mod.get()
            );

            for (u8 a = 0;a < in.funcs[f].func->signature.arg_types.size();a++) {
                func->getArg(a)->setName(format("param_%d", a));
            }
        }

        // translate gjs IR to llvm IR
        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            g.blocks.clear();
            g.links.clear();
            g.target = mod->getFunction(internal_func_name(in.funcs[f].func));
            g.cur_function_idx = f;
            g.cur_instr_idx = in.funcs[f].begin;
            g.cur_block_idx = 0;

            generate_function(g);
        }

        // log llvm IR
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

        // compile
        m_llvm->jit->addIRModule(ThreadSafeModule(std::move(mod), m_llvm->tsc));
        for (u16 f = 0;f < in.funcs.size();f++) {
            if (!in.funcs[f].func) continue;
            in.funcs[f].func->access.entry = m_llvm->jit->lookup(internal_func_name(in.funcs[f].func))->getAddress();
        }
    }

    void win64_backend::log_ir(bool do_log) {
        m_log_ir = do_log;
    }

    void win64_backend::generate_function(w64_gen_ctx& g) {
        using op = compile::operation;
        using var = compile::var;

        LLVMContext* ctx = m_llvm->ctx();

        gen_blocks(g.input.code, g.cur_instr_idx, g.input.funcs[g.cur_function_idx].end, g.blocks, g.links);

        for (u32 b = 0;b < g.blocks.size();b++) {
            g.blocks[b].block = BasicBlock::Create(*ctx, format("L%d", b), g.target);
        }
        
        g.builder = new IRBuilder<>(g.blocks[0].block);

        std::vector<u32> order;
        g.block_order(order);

        for (u32 o = 0;o < order.size();o++) {
            u32 b = order[o];
            g.cur_block_idx = b;
            g.builder->SetInsertPoint(g.blocks[b].block);

            // generate phi nodes
            auto phis = gen_phis(g, b);
            for (auto it = phis.begin();it != phis.end();++it) {
                u64 regId = it->first;
                std::vector<phi_def>& incoming = it->second;
                PHINode* phi = g.builder->CreatePHI(incoming[0].valueType, incoming.size());
                for (u32 p = 0;p < incoming.size();p++) {
                    if (g.blocks[incoming[p].value_source].compiled) {
                        phi->addIncoming(g.blocks[incoming[p].value_source].var_map[regId], g.blocks[incoming[p].when_from].block);
                    } else {
                        g.blocks[incoming[p].value_source].pending_phis.push_back({ regId, phi, g.blocks[incoming[p].when_from].block }); 
                    }
                }
                g.set(regId, phi);
            }

            // generate code
            for (g.cur_instr_idx = g.blocks[b].begin;g.cur_instr_idx <= g.blocks[b].end;g.cur_instr_idx++) {
                 gen_instruction(g);
            }

            // not all blocks end on a branch or jump, but llvm requires
            // links between blocks to be explicit
            if (g.blocks[b].fallthrough) {
                g.builder->CreateBr(g.blocks[b + 1].block);
            }

            // add incoming values to any pending phi nodes
            for (u32 i = 0;i < g.blocks[b].pending_phis.size();i++) {
                auto& pp = g.blocks[b].pending_phis[i];
                pp.phi->addIncoming(g.blocks[b].var_map[pp.regId], pp.when_from);
            }

            g.blocks[b].compiled = true;
        }

        delete g.builder;
        g.builder = nullptr;
    }

    void win64_backend::gen_instruction(w64_gen_ctx& g) {
        using op = compile::operation;
        using var = compile::var;

        const compile::tac_instruction& i = g.input.code[g.cur_instr_idx];
        const var& o1 = i.operands[0];
        const var& o2 = i.operands[1];
        const var& o3 = i.operands[2];
        Value* v1 = o1.valid() ? llvm_value(o1, g) : nullptr;
        Value* v2 = o2.valid() ? llvm_value(o2, g) : nullptr;
        Value* v3 = o3.valid() ? llvm_value(o3, g) : nullptr;

        IRBuilder<>& b = *g.builder;
        LLVMContext* ctx = g.ctx;
        Module* m = g.mod;

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

                Value* ptr = b.CreateIntToPtr(addr, PointerType::get(llvm_type(o1.type(), g), 0));

                g.set(o1.reg_id(), b.CreateLoad(llvm_type(o1.type(), g), ptr));
                break;
            }
            case op::store: {
                Value* ptr = b.CreateIntToPtr(v1, PointerType::get(v2->getType(), 0));
                b.CreateStore(v2, ptr);
                break;
            }
            case op::stack_alloc: {
                Value* ptr = b.CreateAlloca(Type::getInt8Ty(*ctx), v2);
                g.set(o1.reg_id(), b.CreatePtrToInt(ptr, Type::getInt64Ty(*ctx)));
                break;
            }
            case op::stack_free: {
                break;
            }
            case op::module_data: {
                script_module* mod = o2.imm_u() == g.input.mod->id() ? g.input.mod : m_ctx->module(o2.imm_u());
                if (mod) {
                    auto f = METHOD_PTR(script_buffer, data, void*, u64);
                    Value* methodptr = ConstantInt::get(Type::getInt64Ty(*ctx), *(u64*)reinterpret_cast<void*>(&f), false);
                    Value* objptr = ConstantInt::get(Type::getInt64Ty(*ctx), (u64)mod->data(), false);
                    Value* offset = ConstantInt::get(Type::getInt64Ty(*ctx), o3.imm_u(), false);

                    Function* mdata = m->getFunction("__mdata");
                    g.set(o1.reg_id(), b.CreateCall(mdata, {
                        methodptr,
                        objptr,
                        offset
                    }));
                } else {
                    // exception
                }
                break;
            }
            case op::iadd:
            case op::uadd: {
                g.set(o1.reg_id(), b.CreateAdd(v2, v3));
                break;
            }
            case op::isub:
            case op::usub: {
                g.set(o1.reg_id(), b.CreateSub(v2, v3));
                break;
            }
            case op::imul:
            case op::umul: {
                g.set(o1.reg_id(), b.CreateMul(v2, v3));
                break;
            }
            case op::idiv: {
                g.set(o1.reg_id(), b.CreateSDiv(v2, v3));
                break;
            }
            case op::imod: {
                g.set(o1.reg_id(), b.CreateSRem(v2, v3));
                break;
            }
            case op::udiv: {
                g.set(o1.reg_id(), b.CreateUDiv(v2, v3));
                break;
            }
            case op::umod: {
                g.set(o1.reg_id(), b.CreateURem(v2, v3));
                break;
            }
            case op::fadd:
            case op::dadd: {
                g.set(o1.reg_id(), b.CreateFAdd(v2, v3));
                break;
            }
            case op::fsub:
            case op::dsub: {
                g.set(o1.reg_id(), b.CreateFSub(v2, v3));
                break;
            }
            case op::fmul:
            case op::dmul: {
                g.set(o1.reg_id(), b.CreateFMul(v2, v3));
                break;
            }
            case op::fdiv:
            case op::ddiv: {
                g.set(o1.reg_id(), b.CreateFDiv(v2, v3));
                break;
            }
            case op::fmod:
            case op::dmod: {
                g.set(o1.reg_id(), b.CreateFRem(v2, v3));
                break;
            }
            case op::sl: {
                g.set(o1.reg_id(), b.CreateShl(v2, v3));
                break;
            }
            case op::sr: {
                g.set(o1.reg_id(), b.CreateLShr(v2, v3));
                break;
            }
            case op::land: {
                g.set(o1.reg_id(), b.CreateAnd(v2, v3));
                break;
            }
            case op::lor: {
                g.set(o1.reg_id(), b.CreateOr(v2, v3));
                break;
            }
            case op::band: {
                Value* a0 = b.CreateIsNotNull(v2);
                Value* a1 = b.CreateIsNotNull(v3);
                g.set(o1.reg_id(), b.CreateAnd(a0, a1));
                break;
            }
            case op::bor: {
                Value* a0 = b.CreateIsNotNull(v2);
                Value* a1 = b.CreateIsNotNull(v3);
                g.set(o1.reg_id(), b.CreateOr(a0, a1));
                break;
            }
            case op::bxor: {
                g.set(o1.reg_id(), b.CreateXor(v2, v3));
                break;
            }
            case op::ilt: {
                g.set(o1.reg_id(), b.CreateICmpSLT(v2, v3));
                break;
            }
            case op::igt: {
                g.set(o1.reg_id(), b.CreateICmpSGT(v2, v3));
                break;
            }
            case op::ilte: {
                g.set(o1.reg_id(), b.CreateICmpSLE(v2, v3));
                break;
            }
            case op::igte: {
                g.set(o1.reg_id(), b.CreateICmpSGE(v2, v3));
                break;
            }
            case op::incmp:
            case op::uncmp: {
                g.set(o1.reg_id(), b.CreateICmpNE(v2, v3));
                break;
            }
            case op::icmp:
            case op::ucmp: {
                g.set(o1.reg_id(), b.CreateICmpEQ(v2, v3));
                break;
            }
            case op::ult: {
                g.set(o1.reg_id(), b.CreateICmpULT(v2, v3));
                break;
            }
            case op::ugt: {
                g.set(o1.reg_id(), b.CreateICmpUGT(v2, v3));
                break;
            }
            case op::ulte: {
                g.set(o1.reg_id(), b.CreateICmpULE(v2, v3));
                break;
            }
            case op::ugte: {
                g.set(o1.reg_id(), b.CreateICmpUGE(v2, v3));
                break;
            }
            case op::flt:
            case op::dlt: {
                g.set(o1.reg_id(), b.CreateFCmpOLT(v2, v3));
                break;
            }
            case op::fgt:
            case op::dgt: {
                g.set(o1.reg_id(), b.CreateFCmpOGT(v2, v3));
                break;
            }
            case op::flte:
            case op::dlte: {
                g.set(o1.reg_id(), b.CreateFCmpOLE(v2, v3));
                break;
            }
            case op::fgte:
            case op::dgte: {
                g.set(o1.reg_id(), b.CreateFCmpOGE(v2, v3));
                break;
            }
            case op::fncmp:
            case op::dncmp: {
                g.set(o1.reg_id(), b.CreateFCmpONE(v2, v3));
                break;
            }
            case op::fcmp:
            case op::dcmp: {
                g.set(o1.reg_id(), b.CreateFCmpOEQ(v2, v3));
                break;
            }
            case op::eq: {
                g.set(o1.reg_id(), v2);
                break;
            }
            case op::neg: {
                if (o1.type()->is_floating_point) {
                    g.set(o1.reg_id(), b.CreateFNeg(v1));
                } else {
                    g.set(o1.reg_id(), b.CreateNeg(v1));
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
                                g.call_params.insert(g.call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                                g.call_params.insert(g.call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->call_method_func), false));
                                g.call_params.insert(g.call_params.begin(), v1);
                            } else {
                                // return value pointer, func_ptr, call_params...
                                g.call_params.insert(g.call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                                g.call_params.insert(g.call_params.begin(), v1);
                            }
                            b.CreateCall(f, g.call_params);
                        } else if (i.callee->access.wrapped->call_method_func) {
                            // func_ptr, call_params...
                            g.call_params.insert(g.call_params.begin(), ConstantInt::get(Type::getInt64Ty(*ctx), u64(i.callee->access.wrapped->func_ptr), false));
                            if (o1.valid()) g.set(o1.reg_id(), b.CreateCall(f, g.call_params));
                            else b.CreateCall(f, g.call_params);
                        } else {
                            if (o1.valid()) g.set(o1.reg_id(), b.CreateCall(f, g.call_params));
                            else b.CreateCall(f, g.call_params);
                        }
                    } else {
                        if (o1.valid()) g.set(o1.reg_id(), b.CreateCall(f, g.call_params));
                        else b.CreateCall(f, g.call_params);
                    }
                }
                g.call_params.clear();
                break;
            }
            case op::param: {
                g.call_params.push_back(v1);
                break;
            }
            case op::ret: {
                if (g.input.funcs[g.cur_function_idx].func->signature.return_type->size == 0) b.CreateRetVoid();
                else b.CreateRet(v1);
                break;
            }
            case op::cvt: {
                script_type* from = m_ctx->module(extract_left_u32(o2.imm_u()))->types()->get(extract_right_u32(o2.imm_u()));
                script_type* to = m_ctx->module(extract_left_u32(o3.imm_u()))->types()->get(extract_right_u32(o3.imm_u()));
                Value* o = nullptr;
                Type* t = llvm_type(to, g);

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

                if (o) g.set(o1.reg_id(), o);

                break;
            }
            case op::branch: {
                BasicBlock* trueBlock = nullptr;
                BasicBlock* falseBlock = nullptr;
                for (u32 i = 0;i < g.blocks.size();i++) {
                    if (g.blocks[i].begin == o2.imm_u()) {
                        falseBlock = g.blocks[i].block;
                    } else if (g.blocks[i].begin == g.cur_instr_idx + 1) {
                        trueBlock = g.blocks[i].block;
                    }

                    if (trueBlock && falseBlock) break;
                }

                b.CreateCondBr(v1, trueBlock, falseBlock);
                break;
            }
            case op::jump: {
                for (u32 i = 0;i < g.blocks.size();i++) {
                    if (g.blocks[i].begin == o1.imm_u()) {
                        b.CreateBr(g.blocks[i].block);
                        break;
                    }
                }
                break;
            }
            case op::meta_if_branch: break;
            case op::meta_for_loop: break;
            case op::meta_while_loop: break;
            case op::meta_do_while_loop: break;
        }
    }

    void win64_backend::add_bindings_to_module(w64_gen_ctx& g) {
        LLVMContext& ctx = *m_llvm->ctx();
        Function* mdata = Function::Create(
            FunctionType::get(
                Type::getInt64Ty(ctx),
                { Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx) },
                false
            ),
            GlobalValue::LinkageTypes::ExternalLinkage,
            0,
            "__mdata",
            g.mod
        );

        std::vector<script_function*> used_functions;
        robin_hood::unordered_node_set<script_function*> added;
        for (u64 c = 0;c < g.input.code.size();c++) {
            if (g.input.code[c].op == compile::operation::call) {
                if (g.input.code[c].callee->owner == g.input.mod) continue;
                if (!added.count(g.input.code[c].callee)) {
                    added.insert(g.input.code[c].callee);
                    used_functions.push_back(g.input.code[c].callee);
                }
            }
        }
        
        for (u32 f = 0;f < used_functions.size();f++) {
            script_function* func = used_functions[f];
            std::vector<Type*> params;

            Type* ret = llvm_type(used_functions[f]->signature.return_type, g);

            if (func->is_host) {
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
            }

            for (u8 a = 0;a < used_functions[f]->signature.arg_types.size();a++) {
                params.push_back(llvm_type(used_functions[f]->signature.arg_types[a], g));
            }

            FunctionType* sig = FunctionType::get(ret, params, false);
            Function::Create(sig, GlobalValue::LinkageTypes::ExternalLinkage, 0, internal_func_name(used_functions[f]), g.mod);
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