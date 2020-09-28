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
0x003: st64    $s0, 32($sp)
0x004: addu    $a0, $zero, $s0
0x005: addui   $a1, $zero, 145999
0x006: st64    $ra, 40($sp)
0x007: addui   $sp, $sp, 48
0x008: jal     0x7FF7217A240A          ; <- array array::constructor(data)
0x009: subui   $sp, $sp, 48
0x00A: addu    $s1, $v0, $zero
0x00B: ld64    $ra, 40($sp)
0x00C: ld64    $s0, 32($sp)
0x00D: addu    $s2, $s1, $zero
0x00E: addui   $v3, $zero, 145999      ;     x.push(1.23f);
0x00F: st64    $s0, 32($sp)
0x010: addu    $a0, $zero, $s0
0x011: faddi   $f15, $zero, 1.230000
0x012: mffp    $f15, $a1
0x013: st64    $ra, 40($sp)
0x014: addui   $sp, $sp, 48
0x015: jal     0x7FF7217A3427          ; <- void array::push(subtype)
0x016: subui   $sp, $sp, 48
0x017: ld64    $ra, 40($sp)
0x018: ld64    $s0, 32($sp)
0x019: addui   $s2, $zero, 0           ;     x.push(x[0]);
0x01A: cvt.i.u $s2
0x01B: addui   $v3, $zero, 145999
0x01C: addu    $a0, $zero, $s0
0x01D: addu    $a1, $zero, $s2
0x01E: st64    $ra, 32($sp)
0x01F: addui   $sp, $sp, 40
0x020: jal     0x7FF7217A2757          ; <- subtype array::operator [](u32)
0x021: subui   $sp, $sp, 40
0x022: addu    $s2, $v0, $zero
0x023: ld64    $ra, 32($sp)
0x024: ld32    $f0, 0($s2)
0x025: addui   $v3, $zero, 145999
0x026: st32    $f0, 32($sp)
0x027: addu    $a0, $zero, $s0
0x028: mffp    $f0, $a1
0x029: st64    $ra, 36($sp)
0x02A: addui   $sp, $sp, 44
0x02B: jal     0x7FF7217A3427          ; <- void array::push(subtype)
0x02C: subui   $sp, $sp, 44
0x02D: ld64    $ra, 36($sp)
0x02E: ld32    $f0, 32($sp)
0x02F: addui   $s2, $zero, 0           ;     x[0] = 4.56f;
0x030: cvt.i.u $s2
0x031: addui   $v3, $zero, 145999
0x032: addu    $a0, $zero, $s0
0x033: addu    $a1, $zero, $s2
0x034: st64    $ra, 32($sp)
0x035: addui   $sp, $sp, 40
0x036: jal     0x7FF7217A2757          ; <- subtype array::operator [](u32)
0x037: subui   $sp, $sp, 40
0x038: addu    $s2, $v0, $zero
0x039: ld64    $ra, 32($sp)
0x03A: ld32    $f0, 0($s2)
0x03B: faddi   $f0, $zero, 4.560000
0x03C: st64    $f0, 0($s2)
0x03D: addui   $s2, $sp, 32            ;     foo f;
0x03E: st64    $s2, 48($sp)
0x03F: st32    $f0, 56($sp)
0x040: addu    $a0, $zero, $s2
0x041: st64    $ra, 60($sp)
0x042: addui   $sp, $sp, 68
0x043: jal     0x7FF7217A259F          ; <- foo foo::constructor()
0x044: subui   $sp, $sp, 68
0x045: addu    $s1, $v0, $zero
0x046: ld64    $ra, 60($sp)
0x047: ld64    $s2, 48($sp)
0x048: ld32    $f0, 56($sp)
0x049: addu    $s3, $s1, $zero
0x04A: st64    $s2, 48($sp)            ;     f.x = 5;
0x04B: st32    $f0, 56($sp)
0x04C: addu    $a0, $zero, $s2
0x04D: st64    $ra, 60($sp)
0x04E: addui   $sp, $sp, 68
0x04F: jal     0x7FF7217A2EFA          ; <- i32 foo::get_x()
0x050: subui   $sp, $sp, 68
0x051: addu    $s3, $v0, $zero
0x052: ld64    $ra, 60($sp)
0x053: ld64    $s2, 48($sp)
0x054: ld32    $f0, 56($sp)
0x055: addi    $s3, $zero, 5
0x056: st64    $s2, 48($sp)
0x057: st32    $f0, 56($sp)
0x058: addu    $a0, $zero, $s2
0x059: addu    $a1, $zero, $s3
0x05A: st64    $ra, 60($sp)
0x05B: addui   $sp, $sp, 68
0x05C: jal     0x7FF7217A1E79          ; <- i32 foo::set_x(i32)
0x05D: subui   $sp, $sp, 68
0x05E: addu    $s3, $v0, $zero
0x05F: ld64    $ra, 60($sp)
0x060: ld64    $s2, 48($sp)
0x061: ld32    $f0, 56($sp)
0x062: add     $s1, $s3, $zero
0x063: st64    $s2, 48($sp)            ;     f.x += 5;
0x064: st32    $f0, 56($sp)
0x065: addu    $a0, $zero, $s2
0x066: st64    $ra, 60($sp)
0x067: addui   $sp, $sp, 68
0x068: jal     0x7FF7217A2EFA          ; <- i32 foo::get_x()
0x069: subui   $sp, $sp, 68
0x06A: addu    $s1, $v0, $zero
0x06B: ld64    $ra, 60($sp)
0x06C: ld64    $s2, 48($sp)
0x06D: ld32    $f0, 56($sp)
0x06E: addi    $s3, $zero, 5
0x06F: add     $s4, $s1, $s3
0x070: st64    $s2, 48($sp)
0x071: st32    $f0, 56($sp)
0x072: addu    $a0, $zero, $s2
0x073: addu    $a1, $zero, $s4
0x074: st64    $ra, 60($sp)
0x075: addui   $sp, $sp, 68
0x076: jal     0x7FF7217A1E79          ; <- i32 foo::set_x(i32)
0x077: subui   $sp, $sp, 68
0x078: addu    $s4, $v0, $zero
0x079: ld64    $ra, 60($sp)
0x07A: ld64    $s2, 48($sp)
0x07B: ld32    $f0, 56($sp)
0x07C: add     $s3, $s4, $zero
0x07D: fncmp   $v1, $f14, $zero        ;     if (x.length > 0) {
0x07E: bneqz   $v1, 0xA1
0x07F: addui   $v3, $zero, 145999
0x080: st64    $s2, 48($sp)
0x081: st32    $f0, 56($sp)
0x082: addu    $a0, $zero, $s0
0x083: st64    $ra, 60($sp)
0x084: addui   $sp, $sp, 68
0x085: jal     0x7FF7217A410B          ; <- u32 array::get_length()
0x086: subui   $sp, $sp, 68
0x087: addu    $s3, $v0, $zero
0x088: ld64    $ra, 60($sp)
0x089: ld64    $s2, 48($sp)
0x08A: ld32    $f0, 56($sp)
0x08B: addui   $s4, $zero, 0
0x08C: cvt.i.u $s4
0x08D: gt      $s1, $s3, $s4
0x08E: st32    $f0, 48($sp)            ;         print_foo(f);
0x08F: addu    $a0, $zero, $s2
0x090: st64    $ra, 52($sp)
0x091: addui   $sp, $sp, 60
0x092: jal     0x7FF7217A1433          ; <- void print_foo(foo)
0x093: subui   $sp, $sp, 60
0x094: ld64    $ra, 52($sp)
0x095: ld32    $f0, 48($sp)
0x096: addui   $v3, $zero, 145999      ;         x.push(5.55f);
0x097: st32    $f0, 48($sp)
0x098: addu    $a0, $zero, $s0
0x099: faddi   $f15, $zero, 5.550000
0x09A: mffp    $f15, $a1
0x09B: st64    $ra, 52($sp)
0x09C: addui   $sp, $sp, 60
0x09D: jal     0x7FF7217A3427          ; <- void array::push(subtype)
0x09E: subui   $sp, $sp, 60
0x09F: ld64    $ra, 52($sp)
0x0A0: ld32    $f0, 48($sp)
0x0A1: addui   $s2, $zero, 0           ;     for(u8 i = 0;i < 10;i++) {
0x0A2: cvt.i.u $s2
0x0A3: addu    $s1, $s2, $zero
0x0A4: addui   $s2, $zero, 10
0x0A5: cvt.i.u $s2
0x0A6: lt      $s4, $s1, $s2
0x0A7: bneqz   $s4, 0xC0
0x0A8: mtfp    $s1, $f0                ;         x.push(i);
0x0A9: cvt.u.f $f0
0x0AA: addui   $v3, $zero, 145999
0x0AB: st8     $s1, 48($sp)
0x0AC: st8     $s2, 49($sp)
0x0AD: st32    $f0, 50($sp)
0x0AE: st32    $f0, 54($sp)
0x0AF: addu    $a0, $zero, $s0
0x0B0: mffp    $f0, $a1
0x0B1: st64    $ra, 58($sp)
0x0B2: addui   $sp, $sp, 66
0x0B3: jal     0x7FF7217A3427          ; <- void array::push(subtype)
0x0B4: subui   $sp, $sp, 66
0x0B5: ld64    $ra, 58($sp)
0x0B6: ld8     $s1, 48($sp)
0x0B7: ld8     $s2, 49($sp)
0x0B8: ld32    $f0, 50($sp)
0x0B9: ld32    $f0, 54($sp)
0x0BA: addu    $s4, $s1, $zero         ;     for(u8 i = 0;i < 10;i++) {
0x0BB: addui   $s4, $zero, 1
0x0BC: cvt.i.u $s4
0x0BD: addu    $s3, $s1, $s4
0x0BE: addu    $s1, $s3, $zero
0x0BF: jmp     0xA4
0x0C0: addui   $s2, $zero, 0           ;     for(u8 i = 0;i < x.length;i++) {
0x0C1: cvt.i.u $s2
0x0C2: addu    $s1, $s2, $zero
0x0C3: addui   $v3, $zero, 145999
0x0C4: st8     $s1, 48($sp)
0x0C5: st32    $f0, 49($sp)
0x0C6: addu    $a0, $zero, $s0
0x0C7: st64    $ra, 53($sp)
0x0C8: addui   $sp, $sp, 61
0x0C9: jal     0x7FF7217A410B          ; <- u32 array::get_length()
0x0CA: subui   $sp, $sp, 61
0x0CB: addu    $s2, $v0, $zero
0x0CC: ld64    $ra, 53($sp)
0x0CD: ld8     $s1, 48($sp)
0x0CE: ld32    $f0, 49($sp)
0x0CF: addu    $s4, $s2, $zero
0x0D0: lt      $s2, $s1, $s4
0x0D1: bneqz   $s2, 0x0
0x0D2: addu    $s2, $s1, $zero         ;         print_f32(i, x[i]);
0x0D3: addui   $v3, $zero, 145999
0x0D4: st8     $s1, 48($sp)
0x0D5: st32    $f0, 49($sp)
0x0D6: addu    $a0, $zero, $s0
0x0D7: addu    $a1, $zero, $s2
0x0D8: st64    $ra, 53($sp)
0x0D9: addui   $sp, $sp, 61
0x0DA: jal     0x7FF7217A2757          ; <- subtype array::operator [](u32)
0x0DB: subui   $sp, $sp, 61
0x0DC: addu    $s2, $v0, $zero
0x0DD: ld64    $ra, 53($sp)
0x0DE: ld8     $s1, 48($sp)
0x0DF: ld32    $f0, 49($sp)
0x0E0: ld32    $f0, 0($s2)
0x0E1: st8     $s1, 48($sp)
0x0E2: st32    $f0, 49($sp)
0x0E3: addu    $a0, $zero, $s1
0x0E4: fadd    $f0, $zero, $f0
0x0E5: st64    $ra, 53($sp)
0x0E6: addui   $sp, $sp, 61
0x0E7: jal     0x7FF7217A10E1          ; <- void print_f32(u8, f32)
0x0E8: subui   $sp, $sp, 61
0x0E9: ld64    $ra, 53($sp)
0x0EA: ld8     $s1, 48($sp)
0x0EB: ld32    $f0, 49($sp)
0x0EC: addu    $s2, $s1, $zero         ;     for(u8 i = 0;i < x.length;i++) {
0x0ED: addui   $s2, $zero, 1
0x0EE: cvt.i.u $s2
0x0EF: addu    $s4, $s1, $s2
0x0F0: addu    $s1, $s4, $zero
0x0F1: jmp     0xC3
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
1: 0.000000
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
```

Pretty neat
