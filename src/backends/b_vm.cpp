#include <backends/vm.h>

namespace gjs {
    vm_exception::vm_exception(vm_backend* ctx, const std::string& _text) :
        text(_text), raised_from_script(true), line(0), col(0)
    {
        if (ctx->is_executing()) {
            source_ref info = ctx->map()->get((address)ctx->state()->registers[(integer)vm_register::ip]);
            file = info.filename;
            lineText = info.line_text;
            line = info.line;
            col = info.col;
        }
        else raised_from_script = false;
    }

    vm_exception::vm_exception(const std::string& _text) : text(_text), raised_from_script(false), line(0), col(0) {
    }

    vm_exception::~vm_exception() {
    }


    vm_backend::vm_backend(vm_allocator* alloc, u32 stack_size, u32 mem_size) :
        m_vm(this, alloc, stack_size, mem_size), m_instructions(alloc), m_is_executing(false),
        m_log_instructions(false), m_alloc(alloc)
    {
        m_instructions += encode(vm_instruction::term);
        m_map.append("[internal code]", "", 0, 0);
    }

    vm_backend::~vm_backend() {
    }

    void vm_backend::execute(address entry) {
        m_is_executing = true;

        m_vm.execute(m_instructions, entry);

        m_is_executing = false;
    }

    void vm_backend::generate(const ir_code& ir) {
    }

    void vm_backend::call(vm_function* func, void* ret, void** args) {
        /*
        if (sizeof...(args) != signature.arg_locs.size()) {
            throw vm_exception(format(
                "Function '%s' takes %d arguments, %d %s provided",
                name.c_str(),
                signature.arg_locs.size(),
                sizeof...(args),
                (sizeof...(args)) == 1 ? "was" : "were"
            ));
        }

        if (signature.arg_locs.size() > 0) bind::set_arguments(m_ctx, this, 0, args...);
        if (is_host) m_ctx->vm()->call_external(access.wrapped->address);
        else m_ctx->vm()->execute(*m_ctx->code(), access.entry);

        if (signature.return_type->size != 0) {
            nd::from_reg(m_ctx, result, &m_ctx->state()->registers[(u8)signature.return_loc]);
        }
        */
    }
};