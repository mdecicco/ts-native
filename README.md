## Documentation (todo)
- [Overview](https://github.com/mdecicco/gjs/tree/master/src)
    - [Backends](https://github.com/mdecicco/gjs/tree/master/src/backends)
    - [Bind](https://github.com/mdecicco/gjs/tree/master/src/bind)
    - [Common](https://github.com/mdecicco/gjs/tree/master/src/common)
    - [Compiler](https://github.com/mdecicco/gjs/tree/master/src/compiler)
    - [Lexer](https://github.com/mdecicco/gjs/tree/master/src/lexer)
    - [Parser](https://github.com/mdecicco/gjs/tree/master/src/parser)
    - [Util](https://github.com/mdecicco/gjs/tree/master/src/util)
    - [VM](https://github.com/mdecicco/gjs/tree/master/src/vm)

## Setup
1. `git clone --recurse-submodules https://github.com/mdecicco/gjs.git`
2. If building for a 64-bit architecture then do the following when configuring with CMake
    - In the "Specify generator for this project" window
        - Enter `host=x64` in the 'argument to -T' input (or just pass `-Thost=x64` if using the command line)
3. Then before generating the project, set `LLVM_TARGETS_TO_BUILD` to either `'all'` or one or more of the following
    - `X86`
    - `PowerPC` (untested)
    - `AArch64` (untested)
    - `ARM` (untested)
    - `Mips` (untested)
    - `SystemZ` (untested)

    This is (presumably) for enabling JIT compilation support for the selected architecture(s)

## Goals
- To produce a scripting language for games and hobby projects
- Be as similar in syntax and core functionality to JS as possible
- Get reasonably close to the speed of a strictly typed language like C++ that is compiled entirely to machine code, with no garbage collector (so it'll be kind of dangerous)
- Be easy to bind existing C++ interfaces to
- Be easy to debug in realtime
- Be reasonably fast when not compiled to machine code
- Be easy to include into existing projects, with as few dependencies as possible
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

## Supported backends
- VM (software): Should be used while debugging code
- x86_64: JIT Compiles code to native 64 bit x86 code for better performance


<details>
  <summary>An example</summary>

#### The following script code:
```
void it() {
    array<f32> x;
    x.push(1.23f);
    x.push(x[0]);
    x[0] = 4.56f;
    foo f;
    f.x = 5;
    f.x += 5;
    if (x.length > 0) {
        print_foo(f);
        x.push(5.55f);
    }

    for(u8 i = 0;i < 10;i++) {
        x.push(i);
    }

    for(u8 i = 0;i < x.length;i++) {
        print_f32(i, x[i]);
    }
}
```

#### Gets compiled to the following:
```
0x000: term

[void it() -> null]
0x001: addui   $s0, $sp, 0             ;     array<f32> x;
0x002: addui   $v3, $zero, 145999
0x003: addu    $a0, $zero, $s0
0x004: addui   $a1, $zero, 145999
0x005: jal     0x7FF7DFCC2405          ; <- array array::constructor(data)
0x006: addu    $s1, $v0, $zero
0x007: addu    $s2, $s1, $zero
0x008: addui   $v3, $zero, 145999      ;     x.push(1.23f);
0x009: addu    $a0, $zero, $s0
0x00A: faddi   $f15, $zero, 1.230000
0x00B: mffp    $f15, $a1
0x00C: jal     0x7FF7DFCC341D          ; <- void array::push(subtype)
0x00D: addui   $s2, $zero, 0           ;     x.push(x[0]);
0x00E: cvt.i.u $s2
0x00F: addui   $v3, $zero, 145999
0x010: addu    $a0, $zero, $s0
0x011: addu    $a1, $zero, $s2
0x012: jal     0x7FF7DFCC275C          ; <- subtype array::operator [](u32)
0x013: addu    $s2, $v0, $zero
0x014: ld32    $f0, 0($s2)
0x015: addui   $v3, $zero, 145999
0x016: addu    $a0, $zero, $s0
0x017: mffp    $f0, $a1
0x018: jal     0x7FF7DFCC341D          ; <- void array::push(subtype)
0x019: addui   $s2, $zero, 0           ;     x[0] = 4.56f;
0x01A: cvt.i.u $s2
0x01B: addui   $v3, $zero, 145999
0x01C: addu    $a0, $zero, $s0
0x01D: addu    $a1, $zero, $s2
0x01E: jal     0x7FF7DFCC275C          ; <- subtype array::operator [](u32)
0x01F: addu    $s2, $v0, $zero
0x020: ld32    $f0, 0($s2)
0x021: faddi   $f0, $zero, 4.560000
0x022: st32    $f0, 0($s2)
0x023: addui   $s2, $sp, 32            ;     foo f;
0x024: addu    $a0, $zero, $s2
0x025: jal     0x7FF7DFCC25A4          ; <- foo foo::constructor()
0x026: addu    $s1, $v0, $zero
0x027: addu    $s3, $s1, $zero
0x028: addu    $a0, $zero, $s2         ;     f.x = 5;
0x029: jal     0x7FF7DFCC2EFA          ; <- i32 foo::get_x()
0x02A: addu    $s3, $v0, $zero
0x02B: addi    $s3, $zero, 5
0x02C: addu    $a0, $zero, $s2
0x02D: addu    $a1, $zero, $s3
0x02E: jal     0x7FF7DFCC1E6F          ; <- i32 foo::set_x(i32)
0x02F: addu    $s3, $v0, $zero
0x030: add     $s1, $s3, $zero
0x031: addu    $a0, $zero, $s2         ;     f.x += 5;
0x032: jal     0x7FF7DFCC2EFA          ; <- i32 foo::get_x()
0x033: addu    $s1, $v0, $zero
0x034: addi    $s3, $zero, 5
0x035: add     $s4, $s1, $s3
0x036: addu    $a0, $zero, $s2
0x037: addu    $a1, $zero, $s4
0x038: jal     0x7FF7DFCC1E6F          ; <- i32 foo::set_x(i32)
0x039: addu    $s4, $v0, $zero
0x03A: add     $s3, $s4, $zero
0x03B: addui   $v3, $zero, 145999      ;     if (x.length > 0) {
0x03C: addu    $a0, $zero, $s0
0x03D: jal     0x7FF7DFCC40F7          ; <- u32 array::get_length()
0x03E: addu    $s3, $v0, $zero
0x03F: addui   $s4, $zero, 0
0x040: cvt.i.u $s4
0x041: gt      $s1, $s3, $s4
0x042: bneqz   $s1, 0x4A
0x043: addu    $a0, $zero, $s2         ;         print_foo(f);
0x044: jal     0x7FF7DFCC141F          ; <- void print_foo(foo)
0x045: addui   $v3, $zero, 145999      ;         x.push(5.55f);
0x046: addu    $a0, $zero, $s0
0x047: faddi   $f15, $zero, 5.550000
0x048: mffp    $f15, $a1
0x049: jal     0x7FF7DFCC341D          ; <- void array::push(subtype)
0x04A: addui   $s2, $zero, 0           ;     for(u8 i = 0;i < 10;i++) {
0x04B: cvt.i.u $s2
0x04C: addu    $s1, $s2, $zero
0x04D: addui   $s2, $zero, 10
0x04E: cvt.i.u $s2
0x04F: lt      $s4, $s1, $s2
0x050: bneqz   $s4, 0x5D
0x051: mtfp    $s1, $f0                ;         x.push(i);
0x052: cvt.u.f $f0
0x053: addui   $v3, $zero, 145999
0x054: addu    $a0, $zero, $s0
0x055: mffp    $f0, $a1
0x056: jal     0x7FF7DFCC341D          ; <- void array::push(subtype)
0x057: addu    $s4, $s1, $zero         ;     for(u8 i = 0;i < 10;i++) {
0x058: addui   $s4, $zero, 1
0x059: cvt.i.u $s4
0x05A: addu    $s3, $s1, $s4
0x05B: addu    $s1, $s3, $zero
0x05C: jmp     0x4D
0x05D: addui   $s2, $zero, 0           ;     for(u8 i = 0;i < x.length;i++) {
0x05E: cvt.i.u $s2
0x05F: addu    $s1, $s2, $zero
0x060: addui   $v3, $zero, 145999
0x061: addu    $a0, $zero, $s0
0x062: jal     0x7FF7DFCC40F7          ; <- u32 array::get_length()
0x063: addu    $s2, $v0, $zero
0x064: addu    $s4, $s2, $zero
0x065: lt      $s2, $s1, $s4
0x066: bneqz   $s2, 0x0
0x067: addu    $s2, $s1, $zero         ;         print_f32(i, x[i]);
0x068: addui   $v3, $zero, 145999
0x069: addu    $a0, $zero, $s0
0x06A: addu    $a1, $zero, $s2
0x06B: jal     0x7FF7DFCC275C          ; <- subtype array::operator [](u32)
0x06C: addu    $s2, $v0, $zero
0x06D: ld32    $f0, 0($s2)
0x06E: addu    $a0, $zero, $s1
0x06F: fadd    $f0, $zero, $f0
0x070: jal     0x7FF7DFCC10DC          ; <- void print_f32(u8, f32)
0x071: addu    $s2, $s1, $zero         ;     for(u8 i = 0;i < x.length;i++) {
0x072: addui   $s2, $zero, 1
0x073: cvt.i.u $s2
0x074: addu    $s4, $s1, $s2
0x075: addu    $s1, $s4, $zero
0x076: jmp     0x60
```

#### With the following host code
```
class foo {
    public:
        foo() : x(0), y(0), z(0), w(0.0f) {
            printf("Construct foo\n");
        }
        ~foo() {
            printf("Destruct foo\n");
        }

        i32 t(i32 a, i32* b) {
            return printf("%d, %d, %d\n", y, a, *b);
        }

        static void static_func(i32 a) {
            printf("ayyy: %d\n", a);
        }
        f32 ft(f32 a) {
            return s = a;
        }

        operator i32() { return y; }

        i32 get_x() {
            return x;
        }
        i32 set_x(i32 _x) {
            return x = _x;
        }

        i32 x;
        i32 y;
        i32 z;
        f32 w;
        static f32 s;
};
f32 foo::s = 5.5;

void print_foo(const foo& f) {
    printf("foo: %d, %d, %d, %f\n", *f.x, f.y, f.z, f.w);
}

void print_f32(u8 i, f32 f) {
    printf("%d: %f\n", i, f);
}
// ...

vm_allocator* alloc = new basic_malloc_allocator();
vm_backend gen(alloc, 4096, 4096);
script_context ctx(&gen);

try {
    auto f = ctx.bind<foo>("foo");
    f.constructor();
    f.method("t", &foo::t);
    f.method("ft", &foo::ft);
    f.method("operator i32", &foo::operator i32);
    f.method("static_func", &foo::static_func);
    f.prop("x", &foo::get_x, &foo::set_x, bind::property_flags::pf_none);
    f.prop("y", &foo::y, bind::property_flags::pf_none);
    f.prop("z", &foo::z, bind::property_flags::pf_none);
    f.prop("w", &foo::w, bind::property_flags::pf_none);
    f.prop("s", &foo::s, bind::property_flags::pf_none);
    f.finalize();

    ctx.bind(print_foo, "print_foo");
    ctx.bind(print_f32, "print_f32");
} catch (bind_exception& e) {
    printf("%s\n", e.text.c_str());
}

ctx.add_code("test.gjs", src);
```

#### And executed with the following
```
script_function* func = ctx.function("it");
if (func) ctx.call<void>(func, nullptr);
```

#### Producing the following output
```
Construct foo
foo: 10, 0, 0, 0.000000
0: 4.560000
1: 1.230000
2: 5.550000
3: 0.000000
4: 1.000000
5: 2.000000
6: 3.000000
7: 4.000000
8: 5.000000
9: 6.000000
10: 7.000000
11: 8.000000
12: 9.000000
Destruct foo
```

Pretty neat

</details>
