#include <default_steps.h>
#include <instruction_array.h>
#include <source_map.h>
#include <context.h>
#include <vm_function.h>

using namespace std;

namespace gjs {
    using vmi = vm_instruction;
    using vmr = vm_register;

    void remove_instruction(vm_context* ctx, instruction_array& ir, source_map* map, u32 addr, u32 start_addr) {
        ir.remove(addr);
        map->map.erase(map->map.begin() + addr);

        for (u32 a = start_addr;a < ir.size();a++) {
            vmi i = ir[a].instr();

            switch (i) {
                case vmi::beqz:
                case vmi::bneqz:
                case vmi::bgtz:
                case vmi::bgtez:
                case vmi::bltz:
                case vmi::bltez: {
                    vmr reg = ir[a].op_1r();
                    u64 fail_addr = ir[a].imm_u();
                    if (fail_addr > addr) fail_addr--;
                    ir.set(a, encode(i).operand(reg).operand(fail_addr));
                    break;
                }
                case vmi::jmp:
                case vmi::jal: {
                    u64 jmp_addr = ir[a].imm_u();
                    if (jmp_addr > addr && jmp_addr <= ir.size()) jmp_addr--;
                    ir.set(a, encode(i).operand(jmp_addr));
                    break;
                }
                default: {
                }
            }
        }

        vector<vm_function*> funcs = ctx->all_functions();
        for (u16 i = 0;i < funcs.size();i++) {
            if (funcs[i]->is_host) continue;
            if (funcs[i]->access.entry > addr) ctx->set_function_address(funcs[i]->access.entry, funcs[i]->access.entry - 1);
        }
    }

    // de-necessitated 
    void ir_remove_trailing_stack_loads(vm_context* ctx, instruction_array& ir, source_map* map, u32 start_addr) {
        return;
        vmi min_load = vmi::ld8;
        vmi max_load = vmi::ld64;
        vector<u32> remove_addrs;
        vm_function* cur_func = nullptr;
        auto reg_is_safe = [](vmr reg, const vector<vmr>& safe) {
            for (u8 s = 0;s < safe.size();s++) {
                if (safe[s] == reg) return true;
            }
            return false;
        };
        for (u32 a = start_addr;a < ir.size();a++) {
            vm_function* f = ctx->function(a);
            if (f) cur_func = f;
            if (!cur_func) continue;

            vmi i = ir[a].instr();
            if (i >= min_load && i <= max_load && !reg_is_safe(ir[a].op_1r(), { vmr::ra, cur_func->signature.return_loc })) {
                remove_addrs.clear();
                remove_addrs.push_back(a);
                bool function_exits_after = true;
                for (u32 b = a + 1;b < ir.size();b++) {
                    vmi ib = ir[b].instr();
                    if (ib >= min_load && ib <= max_load) {
                        if (!reg_is_safe(ir[a].op_1r(), { vmr::ra, cur_func->signature.return_loc })) {
                            remove_addrs.push_back(b);
                        }
                    } else if (ib == vmi::jmpr) {
                        vmr to = ir[b].op_1r();
                        if (to != vmr::ra) function_exits_after = false;
                        break;
                    } else {
                        function_exits_after = false;
                        break;
                    }
                }

                if (function_exits_after) {
                    for (auto it = remove_addrs.rbegin();it != remove_addrs.rend();it++) {
                        remove_instruction(ctx, ir, map, *it, start_addr);
                    }
                }
            }
        }
    }
};