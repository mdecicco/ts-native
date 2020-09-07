#include <context.h>
#include <vm_function.h>
#include <vm_type.h>
#include <bind.h>
// #include <compiler/compiler.h>
#include <asmjit/asmjit.h>
#include <default_steps.h>
#include <builtin.h>
#include <errors.h>

namespace gjs {
    runtime_exception::runtime_exception(vm_context* ctx, const std::string& _text) : text(_text), raised_from_script(true), line(0), col(0) {
        if (ctx->is_executing()) {
            source_ref info = ctx->map()->get((address)ctx->state()->registers[(integer)vm_register::ip]);
            file = info.filename;
            lineText = info.line_text;
            line = info.line;
            col = info.col;
        }
        else raised_from_script = false;
    }

    runtime_exception::runtime_exception(const std::string& _text) : text(_text), raised_from_script(false), line(0), col(0) {
    }

    runtime_exception::~runtime_exception() {
    }



    vm_context::vm_context(vm_allocator* alloc, u32 stack_size, u32 mem_size) : 
        m_vm(this, alloc, stack_size, mem_size), m_instructions(alloc), m_is_executing(false),
        m_log_instructions(false), m_types(new type_manager(this)), m_alloc(alloc),
        m_pipeline(this)
    {
        m_jit = new asmjit::JitRuntime();
        m_instructions += encode(vm_instruction::term);
        m_map.append("[internal code]", "", 0, 0);

        init_context(this);

        m_pipeline.add_ir_step(ir_remove_trailing_stack_loads);
    }

    vm_context::~vm_context() {
        delete m_jit;
    }

    void vm_context::add(vm_function* func) {
        u64 addr = 0;
        if (func->is_host) addr = func->access.wrapped->address;
        else addr = func->access.entry;

        if (m_funcs_by_addr.count(addr) > 0) {
            throw bind_exception(format("Function '%s' has already been added to the context", func->name.c_str()));
        }

        m_funcs_by_addr[addr] = func;
        m_funcs[func->name] = func;
    }

    vm_function* vm_context::function(const std::string& name) {
        auto it = m_funcs.find(name);
        if (it == m_funcs.end()) return nullptr;
        return it->getSecond();
    }

    vm_function* vm_context::function(u64 address) {
        auto it = m_funcs_by_addr.find(address);
        if (it == m_funcs_by_addr.end()) return nullptr;
        return it->getSecond();
    }

    bool vm_context::set_function_address(u64 old_addr, u64 new_addr) {
        auto it = m_funcs_by_addr.find(old_addr);
        if (it == m_funcs_by_addr.end()) return false;
        if (m_funcs_by_addr.count(new_addr) > 0) return false;
        vm_function* func = it->getSecond();
        if (func->is_host) return false;

        func->access.entry = new_addr;
        m_funcs_by_addr.erase(it);
        m_funcs_by_addr[new_addr] = func;
    }

    std::vector<vm_function*> vm_context::all_functions() {
        std::vector<vm_function*> out;
        for (auto i = m_funcs.begin();i != m_funcs.end();++i) {
            out.push_back(i->getSecond());
        }
        return out;
    }

    std::vector<vm_type*> vm_context::all_types() {
        return m_types->all();
    }

    bool vm_context::add_code(const std::string& filename, const std::string& code) {
        try {
            return m_pipeline.compile(filename, code);
        } catch (error::exception& e) {
            m_pipeline.log()->errors.push_back({ true, e.code, e.message, e.src });
        } catch (std::exception& e) {
            m_pipeline.log()->errors.push_back({ true, error::ecode::unspecified_error, std::string(e.what()), source_ref("[unknown]", "[unknown]", 0, 0) });
        }
        return false;
    }

    void vm_context::execute(address entry) {
        m_is_executing = true;

        m_vm.execute(m_instructions, entry);

        m_is_executing = false;
    }
};
