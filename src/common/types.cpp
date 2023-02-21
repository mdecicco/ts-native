#include <tsn/common/types.h>

namespace tsn {
    value_flags convertPropertyMask(value_flag_mask mask) {
        return {
            (u32)((mask & (u32)vf_read) ? 1 : 0),
            (u32)((mask & (u32)vf_write) ? 1 : 0),
            (u32)((mask & (u32)vf_static) ? 1 : 0),
            (u32)((mask & (u32)vf_pointer) ? 1 : 0)
        };
    }
};