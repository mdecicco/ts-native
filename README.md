## Goals
- To produce a scripting language for games and hobby projects
- Be as similar in syntax and core functionality to JS as possible
- Get reasonably close to the speed of a strictly typed language like C++ that is compiled entirely to machine code, with no garbage collector (so it'll be kind of dangerous)
- Be easy to bind existing C++ interfaces to
- Be easy to debug in realtime
- Be reasonably fast when not compiled to machine code
- Be easy to include into existing projects, with as few dependencies as possible (Only one right now is asmjit)
- To have VM code that is similar to MIPS assembly (because that's what I learned with and x86 seems really confusing)
- To have script code be JIT compiled _entirely_ to native code with no slow checks in place, all checks should happen during compilation
- To have unit tests and be stable at some point

## Why
- v8 is hard to compile and embed in a project without using something like v8pp (which uses specific versions of v8 only)
- v8 isn't fast enough for me and the garbage collector causes frequent and noticeable stutters
- JS classes can't have custom operator methods
- Lua is kind of ugly
- To know everything about the tool so that I can use it to its full potential and modify it to improve that potential if necessary

## Will I finish this
- Probably not, I have like 5 projects that I usually abandon for months (or years) at a time


### To complement this lack of information, here's this example for now
#### The following script code:
```
i32 test(i32 x);
i32 something(i32 arg0, i32 arg1, i32 arg2) {
    i32 x = arg0 + arg1 * 4.0;
    i32 y = x - arg2;
    x = y++;
    for (i32 i = 0;i < 10;i++) {
        x += 3;
    }
    return test(x);
}
i32 test(i32 x) {
    return x * 5;
}
i32 test1(i32 z) {
    return something(z, z + 1, z + 2);
}
i32 main(foo a) {
    a.x = 52;
    a.y *= 5;
    a.z = 2;
    a.w = 61.69;
    print_foo(a);
    return a.t(test1(100));
};
```

#### Gets compiled to the following:
```
0x00: term

[i32 test(i32 arg_0 -> $a0) -> $s7]
0x01: null                            ; i32 test(i32 x);
0x02: muli    $s7, $a0, 5             ;     return x * 5;
0x03: jmpr    $ra

[i32 something(i32 arg_0 -> $a0, i32 arg_1 -> $a1, i32 arg_2 -> $a2) -> $s5]
0x04: null                            ; i32 something(i32 arg0, i32 arg1, i32 arg2) {
0x05: muli    $s6, $a1, 4             ;     i32 x = arg0 + arg1 * 4.0;
0x06: add     $s7, $a0, $s6
0x07: sub     $s6, $s7, $a2           ;     i32 y = x - arg2;
0x08: addi    $s6, $s6, 1             ;     x = y++;
0x09: add     $s7, $zero, $s6
0x0A: addi    $s5, $zero, 0           ;     for (i32 i = 0;i < 10;i++) {
0x0B: lti     $s4, $s5, 10
0x0C: bneqz   $s4, 0x10
0x0D: addi    $s5, $s5, 1
0x0E: addi    $s7, $s7, 3             ;         x += 3;
0x0F: jmp     0xb                     ;     for (i32 i = 0;i < 10;i++) {
0x10: add     $a0, $s7, $zero         ;     return test(x);
0x11: st32    $s6, 0($sp)
0x12: st32    $s7, 4($sp)
0x13: st32    $ra, 8($sp)
0x14: addi    $sp, $sp, 12
0x15: jal     0x1
0x16: subi    $sp, $sp, 12
0x17: add     $s5, $s7, $zero
0x18: ld32    $ra, 8($sp)
0x19: jmpr    $ra

[i32 test1(i32 arg_0 -> $a0) -> $s5]
0x1A: null                            ; i32 test1(i32 z) {
0x1B: addi    $s7, $a0, 1             ;     return something(z, z + 1, z + 2);
0x1C: addi    $s6, $a0, 2
0x1D: add     $a1, $s7, $zero
0x1E: add     $a2, $s6, $zero
0x1F: st32    $a0, 0($sp)
0x20: st32    $s6, 4($sp)
0x21: st32    $s7, 8($sp)
0x22: st32    $ra, 12($sp)
0x23: addi    $sp, $sp, 16
0x24: jal     0x4
0x25: subi    $sp, $sp, 16
0x26: ld32    $ra, 12($sp)
0x27: jmpr    $ra

[i32 main(foo arg_0 -> $a0) -> $s7]
0x28: null                            ; i32 main(foo a) {
0x29: addui   $v1, $a0, 0             ;     a.x = 52;
0x2A: ld64    $s7, 0($v1)
0x2B: ld32    $s7, 0($s7)
0x2C: addi    $s7, $zero, 52
0x2D: ld64    $v1, 0($v1)
0x2E: st32    $s7, 0($v1)
0x2F: addui   $v1, $a0, 8             ;     a.y *= 5;
0x30: ld32    $s7, 0($v1)
0x31: muli    $s7, $s7, 5
0x32: st32    $s7, 0($v1)
0x33: addui   $v1, $a0, 12            ;     a.z = 2;
0x34: ld32    $s7, 0($v1)
0x35: addi    $s7, $zero, 2
0x36: st32    $s7, 0($v1)
0x37: addui   $v1, $a0, 16            ;     a.w = 61.69;
0x38: ld32    $f15, 0($v1)
0x39: faddi   $f15, $zero, 61.689999
0x3A: st32    $f15, 0($v1)
0x3B: st64    $a0, 0($sp)             ;     print_foo(a);
0x3C: st32    $ra, 8($sp)
0x3D: addi    $sp, $sp, 12
0x3E: jal     0xe5d91753
0x3F: subi    $sp, $sp, 12
0x40: ld32    $ra, 8($sp)
0x41: ld64    $a0, 0($sp)
0x42: addi    $s7, $zero, 100         ;     return a.t(test1(100));
0x43: st32    $s7, 8($sp)
0x44: st64    $a0, 0($sp)
0x45: add     $a0, $s7, $zero
0x46: st32    $ra, 12($sp)
0x47: addi    $sp, $sp, 16
0x48: jal     0x1a
0x49: subi    $sp, $sp, 16
0x4A: add     $s6, $s5, $zero
0x4B: ld32    $ra, 12($sp)
0x4C: ld32    $s7, 8($sp)
0x4D: ld64    $a0, 0($sp)
0x4E: add     $a1, $s6, $zero
0x4F: st32    $s6, 8($sp)
0x50: st64    $a0, 0($sp)
0x51: st32    $ra, 12($sp)
0x52: addi    $sp, $sp, 16
0x53: jal     0xcd1ee38
0x54: subi    $sp, $sp, 16
0x55: add     $s7, $v0, $zero
0x56: ld32    $ra, 12($sp)
0x57: jmpr    $ra
```

#### With the following host code
```
class foo {
    public:
        foo(i32* _x) { x = _x; w = 3.0f; }
        ~foo() { }

        i32 t(i32 a) {
            return printf("%d, %d\n", y, a);
        }

        i32* x;
        i32 y;
        i32 z;
        f32 w;
};

void print_foo(const foo& f) {
    printf("foo: %d, %d, %d, %f\n", *f.x, f.y, f.z, f.w);
}

// ...

vm_allocator* alloc = new basic_malloc_allocator();
vm_context ctx(alloc, 4096, 4096);
ctx.log_exceptions(true);
ctx.log_instructions(true);

try {
    auto f = ctx.bind<foo>("foo");
    f.constructor<integer*>();
    f.method("t", &foo::t);
    f.prop("x", &foo::x, bind::property_flags::pf_object_pointer);
    f.prop("y", &foo::y, bind::property_flags::pf_none);
    f.prop("z", &foo::z, bind::property_flags::pf_none);
    f.prop("w", &foo::w, bind::property_flags::pf_none);
    f.finalize();

    ctx.bind(print_foo, "print_foo");
} catch (bind_exception& e) {
    printf("%s\n", e.text.c_str());
}

ctx.add_code("test.gjs", src);
```

#### And executed with the following
```
int x = 5;
foo test(&x);
test.y = 10;
test.z = 4;
integer result = 0;
ctx.function("main")->call(&result, &test);
```

#### Producing the following output
```
0x28: null
0x29: addui   $v1<-858993460>, $a0<215086664>, 0
0x2a: ld64    $s7<-858993460>, 0($v1<215086664>)
0x2b: ld32    $s7<215086628>, 0($s7<215086628>)
0x2c: addi    $s7<5>, $zero<0>, 52
0x2d: ld64    $v1<215086664>, 0($v1<215086664>)
0x2e: st32    $s7<52>, 0($v1<215086628>)
0x2f: addui   $v1<215086628>, $a0<215086664>, 8
0x30: ld32    $s7<52>, 0($v1<215086672>)
0x31: muli    $s7<10>, $s7<10>, 5
0x32: st32    $s7<50>, 0($v1<215086672>)
0x33: addui   $v1<215086672>, $a0<215086664>, 12
0x34: ld32    $s7<50>, 0($v1<215086676>)
0x35: addi    $s7<4>, $zero<0>, 2
0x36: st32    $s7<2>, 0($v1<215086676>)
0x37: addui   $v1<215086676>, $a0<215086664>, 16
0x38: ld32    $f15<14757395478869966848.000000>, 0($v1<215086680>)
0x39: faddi   $f15<1077936128.000000>, $zero<0>, 61.689999
0x3a: st32    $f15<1115079296.000000>, 0($v1<215086680>)
0x3b: st64    $a0<215086664>, 0($sp<0>)
0x3c: st32    $ra<0>, 8($sp<0>)
0x3d: addi    $sp<0>, $sp<0>, 12
0x3e: jal     0xe5d91753
foo: 52, 50, 2, 61.689999
0x3f: subi    $sp<12>, $sp<12>, 12
0x40: ld32    $ra<63>, 8($sp<0>)
0x41: ld64    $a0<215086664>, 0($sp<0>)
0x42: addi    $s7<2>, $zero<0>, 100
0x43: st32    $s7<100>, 8($sp<0>)
0x44: st64    $a0<215086664>, 0($sp<0>)
0x45: add     $a0<215086664>, $s7<100>, $zero<0>
0x46: st32    $ra<0>, 12($sp<0>)
0x47: addi    $sp<0>, $sp<0>, 16
0x48: jal     0x1a
0x1a: null
0x1b: addi    $s7<100>, $a0<100>, 1
0x1c: addi    $s6<-858993460>, $a0<100>, 2
0x1d: add     $a1<-858993460>, $s7<101>, $zero<0>
0x1e: add     $a2<-858993460>, $s6<102>, $zero<0>
0x1f: st32    $a0<100>, 0($sp<16>)
0x20: st32    $s6<102>, 4($sp<16>)
0x21: st32    $s7<101>, 8($sp<16>)
0x22: st32    $ra<73>, 12($sp<16>)
0x23: addi    $sp<16>, $sp<16>, 16
0x24: jal     0x4
0x04: null
0x05: muli    $s6<102>, $a1<101>, 4
0x06: add     $s7<101>, $a0<100>, $s6<404>
0x07: sub     $s6<404>, $s7<504>, $a2<102>
0x08: addi    $s6<402>, $s6<402>, 1
0x09: add     $s7<504>, $zero<0>, $s6<403>
0x0a: addi    $s5<-858993460>, $zero<0>, 0
0x0b: lti     $s4<-858993460>, $s5<0>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<0>, $s5<0>, 1
0x0e: addi    $s7<403>, $s7<403>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<1>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<1>, $s5<1>, 1
0x0e: addi    $s7<406>, $s7<406>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<2>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<2>, $s5<2>, 1
0x0e: addi    $s7<409>, $s7<409>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<3>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<3>, $s5<3>, 1
0x0e: addi    $s7<412>, $s7<412>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<4>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<4>, $s5<4>, 1
0x0e: addi    $s7<415>, $s7<415>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<5>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<5>, $s5<5>, 1
0x0e: addi    $s7<418>, $s7<418>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<6>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<6>, $s5<6>, 1
0x0e: addi    $s7<421>, $s7<421>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<7>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<7>, $s5<7>, 1
0x0e: addi    $s7<424>, $s7<424>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<8>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<8>, $s5<8>, 1
0x0e: addi    $s7<427>, $s7<427>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<9>, 10
0x0c: bneqz   $s4<1>, 0x10
0x0d: addi    $s5<9>, $s5<9>, 1
0x0e: addi    $s7<430>, $s7<430>, 3
0x0f: jmp     0xb
0x0b: lti     $s4<1>, $s5<10>, 10
0x0c: bneqz   $s4<0>, 0x10
0x10: add     $a0<100>, $s7<433>, $zero<0>
0x11: st32    $s6<403>, 0($sp<32>)
0x12: st32    $s7<433>, 4($sp<32>)
0x13: st32    $ra<37>, 8($sp<32>)
0x14: addi    $sp<32>, $sp<32>, 12
0x15: jal     0x1
0x01: null
0x02: muli    $s7<433>, $a0<433>, 5
0x03: jmpr    $ra<22>
0x16: subi    $sp<44>, $sp<44>, 12
0x17: add     $s5<10>, $s7<2165>, $zero<0>
0x18: ld32    $ra<22>, 8($sp<32>)
0x19: jmpr    $ra<37>
0x25: subi    $sp<32>, $sp<32>, 16
0x26: ld32    $ra<37>, 12($sp<16>)
0x27: jmpr    $ra<73>
0x49: subi    $sp<16>, $sp<16>, 16
0x4a: add     $s6<403>, $s5<2165>, $zero<0>
0x4b: ld32    $ra<73>, 12($sp<0>)
0x4c: ld32    $s7<2165>, 8($sp<0>)
0x4d: ld64    $a0<433>, 0($sp<0>)
0x4e: add     $a1<101>, $s6<2165>, $zero<0>
0x4f: st32    $s6<2165>, 8($sp<0>)
0x50: st64    $a0<215086664>, 0($sp<0>)
0x51: st32    $ra<0>, 12($sp<0>)
0x52: addi    $sp<0>, $sp<0>, 16
0x53: jal     0xcd1ee38
50, 2165
0x54: subi    $sp<16>, $sp<16>, 16
0x55: add     $s7<100>, $v0<9>, $zero<0>
0x56: ld32    $ra<84>, 12($sp<0>)
0x57: jmpr    $ra<0>
0x00: term
```

Pretty neat
