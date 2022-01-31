#pragma once

namespace gjs {
    class script_context;
    class script_type;

    // Get script_type that represents some type T
    template <typename T>
    script_type* type_of(script_context* ctx, bool do_throw = false);
};
