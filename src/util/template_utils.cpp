#include <gjs/util/template_utils.hpp>
#include <gjs/common/script_type.h>
#include <gjs/common/function_pointer.h>
#include <gjs/common/script_function.h>
#include <gjs/common/types.h>

namespace gjs {
    void* raw_callback::make(function_pointer* fptr) {
        raw_callback* p = new raw_callback({
            true , // free self
            false, // owns ptr
            false, // owns func
            { nullptr, nullptr },
            fptr
        });

        // printf("Allocated host raw_callback 0x%lX\n", (u64)p);
        return new void*(p);
    }

    void* raw_callback::make(u32 fid, void* data, u64 dataSz) {
        raw_callback* p = new raw_callback({
            false, // free self
            true,  // owns ptr
            false, // owns func
            { nullptr, nullptr },
            new function_pointer(fid, dataSz, data)
        });

        // printf("Allocated raw_callback 0x%lX\n", (u64)p);

        return (void*)p;
    }

    void raw_callback::destroy(raw_callback** cbp) {
        raw_callback* cb = *cbp;
        if (cb) {
            // printf("Destroyed raw_callback 0x%lX\n", (u64)cb);
            if (cb->owns_func && cb->ptr) {
                delete cb->ptr->target->access.wrapped;
                delete cb->ptr->target;
            }

            if (cb->owns_ptr && cb->ptr) delete cb->ptr;
            if (cb->free_self) delete (void*)cbp;
            delete cb;
        }
    }
};