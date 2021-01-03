#include <gjs/common/warnings.h>
#include <stdarg.h>

namespace gjs {
    namespace warning {
        static const char* warn_fmts[] = {
            "No Warning",
            "", // parse warnings
            "", // end parse warnings
            "", // compile warnings
            "", // end compile warnings
        };

        const char* format_str(wcode c) {
            return warn_fmts[(u16)c];
        }
    };
};