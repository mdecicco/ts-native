#pragma once
#include "catch.hpp"
#include <gjs.h>
using namespace gjs;

script_context* simple_vm_context() {
    vm_allocator* alloc = new basic_malloc_allocator();
    return new script_context(new vm_backend(alloc, 4096, 4096));
}

void destroy_simple_vm_context(script_context* ctx) {
    vm_allocator* alloc = ((vm_backend*)ctx->generator())->allocator();
    delete ctx->generator();
    delete alloc;
    delete ctx;
}

bool void_ret_no_args_called = false;
void void_ret_no_args() {
    void_ret_no_args_called = true;
}

struct { int a; float b; int c; } void_ret_with_args_called;
void void_ret_with_args(int a, float b, int c) {
    void_ret_with_args_called = { a, b, c };
}

void bind_tests() {
    script_context* ctx = simple_vm_context();

    SECTION("void return, no args") {
        script_function* func = ctx->bind(void_ret_no_args, "void_ret_no_args");
        ctx->call<void>(func, nullptr);
        REQUIRE(void_ret_no_args_called);
    }

    SECTION("void return, with args") {
        script_function* func = ctx->bind(void_ret_with_args, "void_ret_with_args");
        ctx->call<void>(func, nullptr, 2, 3.14f, 4);
        REQUIRE(void_ret_with_args_called.a == 2);
        REQUIRE(void_ret_with_args_called.b == 3.14f);
        REQUIRE(void_ret_with_args_called.c == 4);
    }

    destroy_simple_vm_context(ctx);
}