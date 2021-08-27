#include <gjs/util/template_utils.hpp>
#include <gjs/common/script_type.h>
#include <gjs/common/function_pointer.h>
#include <gjs/common/types.h>

namespace gjs {
    raw_callback* raw_callback::make(function_pointer* fptr) {
        return new raw_callback({
            false,
            false,
            false,
            { nullptr, nullptr },
            fptr
        });
    }

    void* raw_callback::make(u32 fid, void* data, u64 dataSz) {
        raw_callback* p = new raw_callback({
            true,
            true,
            false,
            { nullptr, nullptr },
            new function_pointer(fid, dataSz)
        });
        void** out = new void*;
        *out = p;

        return out;
    }
};