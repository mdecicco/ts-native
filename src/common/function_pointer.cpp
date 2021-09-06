#include <gjs/common/function_pointer.h>
#include <gjs/backends/backend.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_module.h>
#include <gjs/common/errors.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/common/script_context.h>
#include <gjs/builtin/builtin.h>

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
        if (data) {
            u8* dptr = (u8*)data;
            u32 captureCount = *(u32*)dptr;
            if (captureCount > 0) {
                dptr += sizeof(u32);
                script_type** captureTypes = new script_type*[captureCount];

                for (u32 i = 0;i < captureCount;i++) {
                    u64 moduletypeId = *(u64*)dptr;
                    u32 moduleId = extract_left_u32(moduletypeId);
                    u32 typeId = extract_right_u32(moduletypeId);
                    script_module* mod = target->ctx()->module(moduleId);
                    captureTypes[i] = mod->types()->get(typeId);
                    dptr += sizeof(u64);
                }

                for (u32 i = 0;i < captureCount;i++) {
                    if (captureTypes[i]->destructor) {
                        captureTypes[i]->destructor->call((void*)dptr);
                    }
                    dptr += captureTypes[i]->size;
                }
            }

            script_free(data);
        }
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
