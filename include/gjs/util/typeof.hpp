namespace gjs {
    template <typename T>
    script_type* type_of(script_context* ctx, bool do_throw) {
        return ctx->types()->get<T>(do_throw);
    }
};
