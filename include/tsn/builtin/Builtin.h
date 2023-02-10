#pragma once

namespace tsn {
    class Context;
    void AddBuiltInBindings(Context* ctx);
    void BindMemory(Context* ctx);
};