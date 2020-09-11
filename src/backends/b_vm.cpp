#include <backends/vm.h>
#include <compiler/tac.h>
#include <backends/register_allocator.h>

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

    void vm_backend::generate(const compilation_output& in) {
        using op = compile::operation;
        using vmi = vm_instruction;
        using vmr = vm_register;

        for (u32 i = 0;i < in.code.size();i++) {
            switch(in.code[i].op) {
                case op::null: {
                    break;
                }
                case op::load: {
                    break;
                }
                case op::store: {
                    break;
                }
                case op::stack_alloc: {
                    break;
                }
                case op::stack_free: {
                    break;
                }
                case op::iadd: {
                    break;
                }
                case op::isub: {
                    break;
                }
                case op::imul: {
                    break;
                }
                case op::idiv: {
                    break;
                }
                case op::imod: {
                    break;
                }
                case op::uadd: {
                    break;
                }
                case op::usub: {
                    break;
                }
                case op::umul: {
                    break;
                }
                case op::udiv: {
                    break;
                }
                case op::umod: {
                    break;
                }
                case op::fadd: {
                    break;
                }
                case op::fsub: {
                    break;
                }
                case op::fmul: {
                    break;
                }
                case op::fdiv: {
                    break;
                }
                case op::fmod: {
                    break;
                }
                case op::dadd: {
                    break;
                }
                case op::dsub: {
                    break;
                }
                case op::dmul: {
                    break;
                }
                case op::ddiv: {
                    break;
                }
                case op::dmod: {
                    break;
                }
                case op::sl: {
                    break;
                }
                case op::sr: {
                    break;
                }
                case op::land: {
                    break;
                }
                case op::lor: {
                    break;
                }
                case op::band: {
                    break;
                }
                case op::bor: {
                    break;
                }
                case op::bxor: {
                    break;
                }
                case op::ilt: {
                    break;
                }
                case op::igt: {
                    break;
                }
                case op::ilte: {
                    break;
                }
                case op::igte: {
                    break;
                }
                case op::incmp: {
                    break;
                }
                case op::icmp: {
                    break;
                }
                case op::ult: {
                    break;
                }
                case op::ugt: {
                    break;
                }
                case op::ulte: {
                    break;
                }
                case op::ugte: {
                    break;
                }
                case op::uncmp: {
                    break;
                }
                case op::ucmp: {
                    break;
                }
                case op::flt: {
                    break;
                }
                case op::fgt: {
                    break;
                }
                case op::flte: {
                    break;
                }
                case op::fgte: {
                    break;
                }
                case op::fncmp: {
                    break;
                }
                case op::fcmp: {
                    break;
                }
                case op::dlt: {
                    break;
                }
                case op::dgt: {
                    break;
                }
                case op::dlte: {
                    break;
                }
                case op::dgte: {
                    break;
                }
                case op::dncmp: {
                    break;
                }
                case op::dcmp: {
                    break;
                }
                case op::eq: {
                    break;
                }
                case op::neg: {
                    break;
                }
                case op::call: {
                    break;
                }
                case op::param: {
                    break;
                }
                case op::ret: {
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
    
    u16 vm_backend::gp_count() const {
        return 8;
    }

    u16 vm_backend::fp_count() const {
        return 16;
    }

    void vm_backend::call(script_function* func, void* ret, void** args) {
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