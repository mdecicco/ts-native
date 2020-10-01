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

struct { int a; int b; int c; } void_ret_with_int_args_called;
void void_ret_with_int_args(int a, int b, int c) {
    void_ret_with_int_args_called = { a, b, c };
}

struct { float a; float b; float c; } void_ret_with_float_args_called;
void void_ret_with_float_args(float a, float b, float c) {
    void_ret_with_float_args_called = { a, b, c };
}

struct { double a; double b; double c; } void_ret_with_double_args_called;
void void_ret_with_double_args(double a, double b, double c) {
    void_ret_with_double_args_called = { a, b, c };
}

int int_ret() { return 5; }
float float_ret() { return 5.0f; }
double double_ret() { return 5.0; }

template <typename T>
T passthrough(T arg) { return arg; }

struct vec3 { float x, y, z; };
struct mat3 { vec3 x, y, z; };

vec3 struct_ret() { return { 1.0f, 2.0f, 3.0f }; }

vec3 struct_arg_called;
void struct_arg(const vec3& arg) { struct_arg_called = arg; }

SCENARIO("Binding features behave as expected", "[gjs::bind]") {
    script_context* ctx = simple_vm_context();

    SECTION("void return, no args") {
        script_function* func = ctx->bind(void_ret_no_args, "void_ret_no_args");
        ctx->call<void>(func, nullptr);
        REQUIRE(void_ret_no_args_called);
    }

    SECTION("void return, with int args") {
        script_function* func = ctx->bind(void_ret_with_int_args, "void_ret_with_int_args");
        ctx->call<void>(func, nullptr, 2, 3, 4);
        REQUIRE(void_ret_with_int_args_called.a == 2);
        REQUIRE(void_ret_with_int_args_called.b == 3);
        REQUIRE(void_ret_with_int_args_called.c == 4);
    }

    SECTION("void return, with double args") {
        script_function* func = ctx->bind(void_ret_with_double_args, "void_ret_with_double_args");
        ctx->call<void>(func, nullptr, 2.345, 3.456, 4.567);
        REQUIRE(void_ret_with_double_args_called.a == 2.345);
        REQUIRE(void_ret_with_double_args_called.b == 3.456);
        REQUIRE(void_ret_with_double_args_called.c == 4.567);
    }

    SECTION("int return") {
        script_function* func = ctx->bind(int_ret, "int_ret");
        int ret;
        ctx->call(func, &ret);
        REQUIRE(ret == 5);
    }

    SECTION("float return") {
        script_function* func = ctx->bind(float_ret, "float_ret");
        float ret;
        ctx->call(func, &ret);
        REQUIRE(ret == 5.0f);
    }

    SECTION("double return") {
        script_function* func = ctx->bind(double_ret, "double_ret");
        double ret;
        ctx->call(func, &ret);
        REQUIRE(ret == 5.0);
    }

    SECTION("int return, with args") {
        script_function* func = ctx->bind(passthrough<int>, "int_ret_with_args");
        int ret;
        ctx->call(func, &ret, 5);
        REQUIRE(ret == 5);
    }

    SECTION("float return, with args") {
        script_function* func = ctx->bind(passthrough<float>, "float_ret_with_args");
        float ret;
        ctx->call(func, &ret, 5.0f);
        REQUIRE(ret == 5.0f);
    }

    SECTION("double return, with args") {
        script_function* func = ctx->bind(passthrough<double>, "double_ret_with_args");
        double ret;
        ctx->call(func, &ret, 5.0);
        REQUIRE(ret == 5.0);
    }

    SECTION("Binding function with unbound return type throws exception") {
        bool threw = false;
        try {
            ctx->bind(struct_ret, "should_fail");
        } catch (const bind_exception& e) {
            threw = true;
        }
        REQUIRE(threw == true);
    }

    SECTION("Binding function with unbound argument type throws exception") {
        bool threw = false;
        try {
            ctx->bind(struct_arg, "should_fail");
        } catch (const bind_exception& e) {
            threw = true;
        }
        REQUIRE(threw == true);
    }
    
    SECTION("Binding struct with unbound field type throws exception") {
        bool threw = false;
        try {
            ctx->bind<mat3>("mat3").prop("x", &mat3::x).prop("y", &mat3::y).prop("z", &mat3::z).finalize();
        } catch (const bind_exception& e) {
            threw = true;
        }
        REQUIRE(threw == true);
    }

    ctx->bind<vec3>("vec3").prop("x", &vec3::x).prop("y", &vec3::y).prop("z", &vec3::z).finalize();

    SECTION("Passing struct by value throws exception when bind attempted") {
        bool threw = false;
        try {
            ctx->bind(passthrough<vec3>, "should_fail");
        } catch (const bind_exception& e) {
            threw = true;
        }
        REQUIRE(threw == true);
    }
    
    SECTION("struct/class reference/pointer argument") {
        script_function* func = ctx->bind(struct_arg, "struct_arg");
        ctx->call<void>(func, nullptr, vec3({ 1.0f, 2.0f, 3.0f }));
        REQUIRE(struct_arg_called.x == 1.0f);
        REQUIRE(struct_arg_called.y == 2.0f);
        REQUIRE(struct_arg_called.z == 3.0f);
    }

    SECTION("struct/class return (by value)") {
        script_function* func = ctx->bind(struct_ret, "struct_ret");
        vec3 ret;
        ctx->call(func, &ret);
        REQUIRE(ret.x == 1.0f);
        REQUIRE(ret.y == 2.0f);
        REQUIRE(ret.z == 3.0f);
    }

    destroy_simple_vm_context(ctx);
}