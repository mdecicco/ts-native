#include <gjs/common/function_pointer.h>
#include <gjs/common/script_function.h>
#include <gjs/common/errors.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/common/script_context.h>

using namespace gjs::error;

namespace gjs {
    function_pointer::function_pointer(script_function* _target, u64 dataSize, void* _data) {
        m_this = nullptr;
        target = _target;
        data = _data;
    }

    function_pointer::function_pointer(u32 func_id, u64 dataSize, void* _data) {
        m_this = nullptr;
        target = script_context::current()->function(func_id);
        data = _data;
    }

    function_pointer::~function_pointer() {
        data = nullptr;
    }

    void function_pointer::bind_this(void* self) {
        if (target->is_static || !target->is_method_of) {
            throw runtime_exception(ecode::r_fp_bind_self_thiscall_only);
        }

        m_this = self;
    }

    void* function_pointer::self_obj() const {
        return m_this;
    }
};
