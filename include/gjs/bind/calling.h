#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_object.h>

namespace gjs {
    struct raw_callback;

    template <typename... Args>
    script_object call(script_function* func, void* self = nullptr, Args... args);

    template <typename... Args>
    script_object call(u64 moduletype_id, script_function* func, void* self = nullptr, Args... args);

    template <typename... Args>
    script_object call(raw_callback* cb, Args... args);

    template <typename... Args>
    script_object instantiate(script_type* type, Args... args);

    template <typename... Args>
    script_object construct_at(script_type* type, void* dest, Args... args);
};

#include <gjs/bind/calling.inl>