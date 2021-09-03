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

    This is (presumably) for enabling JIT compilation support for the selected architecture(s). The more you select, the older you'll be when the compilation finishes.

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
import { vec2f } from 'math';

class some_guy {
	constructor(f32 x, f32 y) : v(vec2f(x, y)) { }
	constructor(some_guy g) : v(vec2f(g.v)) { }

	void print() {
		print('vec2f(' + this.v.x.toFixed(2) + ', ' + this.v.y.toFixed(2) + ')');
	}

	vec2f v;
};

i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
	return a + b;
};

i32 x = testCb(cb);
print(x.toFixed(2));

some_guy a = some_guy(2, 2);
a.print();

array<some_guy> arr;
for (u32 i = 0;i < 10;i++) {
	arr.push(some_guy(i, i));
}

for (u32 i = 0;i < arr.length;i++) arr[i].print();
```

#### Gets compiled to the following:
```
 0x000: term

[void test::__init__() -> null]
 0x001: st64    $ra, 0($sp)
 0x002: st64    $s3, 8($sp)
 0x003: st64    $s5, 16($sp)
 0x004: st64    $a0, 24($sp)
 0x005: st64    $a2, 32($sp)
 0x006: st64    $s1, 40($sp)
 0x007: st64    $fa1, 48($sp)
 0x008: st64    $a1, 56($sp)
 0x009: st64    $f0, 64($sp)
 0x00A: st64    $s4, 72($sp)
 0x00B: st64    $s2, 80($sp)
 0x00C: st64    $s0, 88($sp)
 0x00D: st64    $fa0, 96($sp)
 0x00E: addui   $v3, $zero, 2834382347  ; import { vec2f } from 'math';
 0x00F: mptr    $s0, 0x0
 0x010: addui   $s1, $zero, 0           ; i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
 0x011: addui   $a0, $zero, 3453803796
 0x012: addu    $a1, $zero, $s1
 0x013: addui   $a2, $zero, 0
 0x014: addui   $sp, $sp, 104
 0x015: jal     data __global__::$makefunc(u32,data,u64)
 0x016: subui   $sp, $sp, 104
 0x017: addu    $s1, $v0, $zero
 0x018: addu    $s2, $s1, $zero
 0x019: addu    $s1, $s2, $zero         ; import { vec2f } from 'math';
 0x01A: st64    $s1, 0($s0)
 0x01B: addui   $v3, $zero, 2834382347
 0x01C: mptr    $s0, 0x8
 0x01D: addu    $a0, $zero, $s1         ; i32 x = testCb(cb);
 0x01E: addui   $sp, $sp, 104
 0x01F: jal     i32 __global__::testCb(i32(i32,i32))
 0x020: subui   $sp, $sp, 104
 0x021: addu    $s1, $v0, $zero
 0x022: add     $s2, $s1, $zero         ; import { vec2f } from 'math';
 0x023: st32    $s2, 0($s0)
 0x024: addui   $s0, $sp, 104           ; print(x.toFixed(2));
 0x025: addu    $a0, $zero, $s0
 0x026: addu    $a1, $zero, $s2
 0x027: addui   $a2, $zero, 2
 0x028: addui   $sp, $sp, 128
 0x029: jal     string __global__::i32::toFixed($data,i32,u8)
 0x02A: subui   $sp, $sp, 128
 0x02B: addu    $a0, $zero, $s0
 0x02C: addui   $sp, $sp, 128
 0x02D: jal     u32 __global__::print(string)
 0x02E: subui   $sp, $sp, 128
 0x02F: addu    $s2, $v0, $zero
 0x030: addu    $a0, $zero, $s0
 0x031: addui   $sp, $sp, 128
 0x032: jal     void __global__::string::destructor($string)
 0x033: subui   $sp, $sp, 128
 0x034: addui   $s0, $sp, 104           ; some_guy a = some_guy(2, 2);
 0x035: addu    $a0, $zero, $s0
 0x036: faddi   $fa0, $zero, 2.000000
 0x037: faddi   $fa1, $zero, 2.000000
 0x038: addui   $sp, $sp, 112
 0x039: jal     void test::some_guy::constructor($some_guy,f32,f32)
 0x03A: subui   $sp, $sp, 112
 0x03B: addui   $v3, $zero, 2834382347
 0x03C: mptr    $s2, 0xC
 0x03D: addu    $a0, $zero, $s2
 0x03E: addu    $a1, $zero, $s0
 0x03F: addui   $sp, $sp, 112
 0x040: jal     void test::some_guy::constructor($some_guy,some_guy)
 0x041: subui   $sp, $sp, 112
 0x042: addu    $a0, $zero, $s2         ; a.print();
 0x043: addui   $sp, $sp, 112
 0x044: jal     void test::some_guy::print($some_guy)
 0x045: subui   $sp, $sp, 112
 0x046: addui   $v3, $zero, 2834382347  ; array<some_guy> arr;
 0x047: mptr    $s2, 0x14
 0x048: addui   $v3, $zero, -6273164584710744737
 0x049: addu    $a0, $zero, $s2
 0x04A: addui   $a1, $zero, -6273164584710744737
 0x04B: addui   $sp, $sp, 112
 0x04C: jal     void __global__::array::constructor($array,$u64)
 0x04D: subui   $sp, $sp, 112
 0x04E: addui   $v3, $zero, 2834382347  ; for (u32 i = 0;i < 10;i++) {
 0x04F: mptr    $s1, 0x34
 0x050: addui   $s3, $zero, 0
 0x051: st32    $s3, 0($s1)
 0x052: lti     $s4, $s3, 10
 0x053: bneqz   $s4, 0x6A
 0x054: addui   $s4, $sp, 112           ;       arr.push(some_guy(i, i));
 0x055: mtfp    $s3, $f0
 0x056: cvt.u.f $f0
 0x057: mtfp    $s3, $f0
 0x058: cvt.u.f $f0
 0x059: addu    $a0, $zero, $s4
 0x05A: fadd    $fa0, $zero, $f0
 0x05B: fadd    $fa1, $zero, $f0
 0x05C: addui   $sp, $sp, 120
 0x05D: jal     void test::some_guy::constructor($some_guy,f32,f32)
 0x05E: subui   $sp, $sp, 120
 0x05F: addui   $v3, $zero, -6273164584710744737
 0x060: addu    $a0, $zero, $s2
 0x061: addu    $a1, $zero, $s4
 0x062: addui   $sp, $sp, 120
 0x063: jal     void __global__::array::push($array,subtype)
 0x064: subui   $sp, $sp, 120
 0x065: addu    $s4, $s3, $zero         ; for (u32 i = 0;i < 10;i++) {
 0x066: addui   $s4, $s3, 1
 0x067: addu    $s3, $s4, $zero
 0x068: st32    $s3, 0($s1)
 0x069: jmp     0x52
 0x06A: addui   $v3, $zero, 2834382347  ; for (u32 i = 0;i < arr.length;i++) arr[i].print();
 0x06B: mptr    $s3, 0x38
 0x06C: addui   $s1, $zero, 0
 0x06D: st32    $s1, 0($s3)
 0x06E: addui   $v3, $zero, -6273164584710744737
 0x06F: addu    $a0, $zero, $s2
 0x070: addui   $sp, $sp, 112
 0x071: jal     u32 __global__::array::get_length($array)
 0x072: subui   $sp, $sp, 112
 0x073: addu    $s4, $v0, $zero
 0x074: lt      $s5, $s1, $s4
 0x075: bneqz   $s5, 0x86
 0x076: addui   $v3, $zero, -6273164584710744737
 0x077: addu    $a0, $zero, $s2
 0x078: addu    $a1, $zero, $s1
 0x079: addui   $sp, $sp, 112
 0x07A: jal     subtype __global__::array::operator []($array,u32)
 0x07B: subui   $sp, $sp, 112
 0x07C: addu    $s5, $v0, $zero
 0x07D: addu    $a0, $zero, $s5
 0x07E: addui   $sp, $sp, 112
 0x07F: jal     void test::some_guy::print($some_guy)
 0x080: subui   $sp, $sp, 112
 0x081: addu    $s5, $s1, $zero
 0x082: addui   $s5, $s1, 1
 0x083: addu    $s1, $s5, $zero
 0x084: st32    $s1, 0($s3)
 0x085: jmp     0x6E
 0x086: ld64    $ra, 0($sp)             ; import { vec2f } from 'math';
 0x087: ld64    $s3, 8($sp)             ;
 0x088: ld64    $s5, 16($sp)
 0x089: ld64    $a0, 24($sp)
 0x08A: ld64    $a2, 32($sp)
 0x08B: ld64    $s1, 40($sp)
 0x08C: ld64    $fa1, 48($sp)
 0x08D: ld64    $a1, 56($sp)
 0x08E: ld64    $f0, 64($sp)
 0x08F: ld64    $s4, 72($sp)
 0x090: ld64    $s2, 80($sp)
 0x091: ld64    $s0, 88($sp)
 0x092: ld64    $fa0, 96($sp)
 0x093: jmpr    $ra

[i32 test::lambda_0(i32 arg_0 -> $a0, i32 arg_1 -> $a1) -> $v0]
 0x094: st64    $s0, 0($sp)
 0x095: add     $s0, $a0, $a1           ; i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
 0x096: addu    $v0, $s0, $zero         ;       return a + b;
 0x097: ld64    $s0, 0($sp)
 0x098: jmpr    $ra

[void test::some_guy::constructor(some_guy @this -> $a0, f32 arg_1 -> $fa0, f32 arg_2 -> $fa1) -> null]
 0x099: st64    $ra, 0($sp)             ; i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
 0x09A: st64    $a1, 8($sp)
 0x09B: st64    $s0, 16($sp)            ;       constructor(f32 x, f32 y) : v(vec2f(x, y)) { }
 0x09C: st64    $s1, 24($sp)
 0x09D: addui   $s0, $a0, 0
 0x09E: addui   $s1, $sp, 32
 0x09F: addu    $a0, $zero, $s1
 0x0A0: fadd    $fa0, $zero, $fa0
 0x0A1: fadd    $fa1, $zero, $fa1
 0x0A2: addui   $sp, $sp, 40
 0x0A3: jal     void math::vec2f::constructor($vec2f,f32,f32)
 0x0A4: subui   $sp, $sp, 40
 0x0A5: addu    $a0, $zero, $s0
 0x0A6: addu    $a1, $zero, $s1
 0x0A7: addui   $sp, $sp, 40
 0x0A8: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x0A9: subui   $sp, $sp, 40
 0x0AA: addu    $a0, $zero, $s1
 0x0AB: addui   $sp, $sp, 40
 0x0AC: jal     void math::vec2f::destructor($vec2f)
 0x0AD: subui   $sp, $sp, 40
 0x0AE: ld64    $ra, 0($sp)
 0x0AF: ld64    $a1, 8($sp)
 0x0B0: ld64    $s0, 16($sp)
 0x0B1: ld64    $s1, 24($sp)
 0x0B2: jmpr    $ra

[void test::some_guy::constructor(some_guy @this -> $a0, some_guy arg_1 -> $a1) -> null]
 0x0B3: st64    $ra, 0($sp)
 0x0B4: st64    $s2, 8($sp)
 0x0B5: st64    $s0, 16($sp)
 0x0B6: st64    $s1, 24($sp)            ;       constructor(some_guy g) : v(vec2f(g.v)) { }
 0x0B7: addui   $s0, $a0, 0
 0x0B8: addui   $s1, $a1, 0
 0x0B9: addu    $s2, $s1, $zero
 0x0BA: addui   $s1, $sp, 32
 0x0BB: addu    $a0, $zero, $s1
 0x0BC: addu    $a1, $zero, $s2
 0x0BD: addui   $sp, $sp, 40
 0x0BE: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x0BF: subui   $sp, $sp, 40
 0x0C0: addu    $a0, $zero, $s0
 0x0C1: addu    $a1, $zero, $s1
 0x0C2: addui   $sp, $sp, 40
 0x0C3: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x0C4: subui   $sp, $sp, 40
 0x0C5: addu    $a0, $zero, $s1
 0x0C6: addui   $sp, $sp, 40
 0x0C7: jal     void math::vec2f::destructor($vec2f)
 0x0C8: subui   $sp, $sp, 40
 0x0C9: ld64    $ra, 0($sp)
 0x0CA: ld64    $s2, 8($sp)
 0x0CB: ld64    $s0, 16($sp)
 0x0CC: ld64    $s1, 24($sp)
 0x0CD: jmpr    $ra

[void test::some_guy::print(some_guy @this -> $a0) -> null]
 0x0CE: st64    $ra, 8($sp)
 0x0CF: st64    $a1, 16($sp)
 0x0D0: st64    $s3, 24($sp)
 0x0D1: st64    $s5, 32($sp)
 0x0D2: st64    $f0, 40($sp)            ;       void print() {
 0x0D3: st64    $a2, 48($sp)
 0x0D4: st64    $s1, 56($sp)
 0x0D5: st64    $s4, 64($sp)
 0x0D6: st64    $s6, 72($sp)
 0x0D7: st64    $s7, 80($sp)
 0x0D8: st64    $s2, 88($sp)
 0x0D9: st64    $s0, 96($sp)
 0x0DA: addui   $v3, $zero, 2834382347
 0x0DB: mptr    $s0, 0x3C
 0x0DC: addui   $s1, $sp, 104
 0x0DD: st64    $a0, 128($sp)
 0x0DE: addu    $a0, $zero, $s1         ;               print('vec2f(' + this.v.x.toFixed(2) + ', ' + this.v.y.toFixed(2) + ')');
 0x0DF: addu    $a1, $zero, $s0
 0x0E0: addui   $a2, $zero, 6
 0x0E1: addui   $sp, $sp, 136
 0x0E2: jal     void __global__::string::constructor($string,data,u32)
 0x0E3: subui   $sp, $sp, 136
 0x0E4: ld64    $a0, 128($sp)
 0x0E5: addui   $s0, $a0, 0
 0x0E6: addu    $s2, $s0, $zero
 0x0E7: addui   $s0, $s2, 0
 0x0E8: ld32    $f0, 0($s2)
 0x0E9: addui   $s2, $sp, 128
 0x0EA: st64    $a0, 152($sp)
 0x0EB: addu    $a0, $zero, $s2
 0x0EC: fadd    $fa0, $zero, $f0
 0x0ED: addui   $a1, $zero, 2
 0x0EE: addui   $sp, $sp, 160
 0x0EF: jal     string __global__::f32::toFixed($data,f32,u8)
 0x0F0: subui   $sp, $sp, 160
 0x0F1: ld64    $a0, 152($sp)
 0x0F2: addui   $s0, $sp, 152
 0x0F3: st64    $a0, 176($sp)
 0x0F4: addu    $a0, $zero, $s1
 0x0F5: addu    $a1, $zero, $s0
 0x0F6: addu    $a2, $zero, $s2
 0x0F7: addui   $sp, $sp, 184
 0x0F8: jal     string __global__::string::operator +($string,$data,string)
 0x0F9: subui   $sp, $sp, 184
 0x0FA: ld64    $a0, 176($sp)
 0x0FB: addui   $v3, $zero, 2834382347
 0x0FC: mptr    $s3, 0x42
 0x0FD: addui   $s4, $sp, 176
 0x0FE: st64    $a0, 200($sp)
 0x0FF: addu    $a0, $zero, $s4
 0x100: addu    $a1, $zero, $s3
 0x101: addui   $a2, $zero, 2
 0x102: addui   $sp, $sp, 208
 0x103: jal     void __global__::string::constructor($string,data,u32)
 0x104: subui   $sp, $sp, 208
 0x105: ld64    $a0, 200($sp)
 0x106: addui   $s3, $sp, 200
 0x107: st64    $a0, 224($sp)
 0x108: addu    $a0, $zero, $s0
 0x109: addu    $a1, $zero, $s3
 0x10A: addu    $a2, $zero, $s4
 0x10B: addui   $sp, $sp, 232
 0x10C: jal     string __global__::string::operator +($string,$data,string)
 0x10D: subui   $sp, $sp, 232
 0x10E: ld64    $a0, 224($sp)
 0x10F: addui   $s5, $a0, 0
 0x110: addu    $s6, $s5, $zero
 0x111: addui   $s5, $s6, 4
 0x112: ld32    $f0, 4($s6)
 0x113: addui   $s6, $sp, 224
 0x114: addu    $a0, $zero, $s6
 0x115: fadd    $fa0, $zero, $f0
 0x116: addui   $a1, $zero, 2
 0x117: addui   $sp, $sp, 248
 0x118: jal     string __global__::f32::toFixed($data,f32,u8)
 0x119: subui   $sp, $sp, 248
 0x11A: addui   $s5, $sp, 248
 0x11B: addu    $a0, $zero, $s3
 0x11C: addu    $a1, $zero, $s5
 0x11D: addu    $a2, $zero, $s6
 0x11E: addui   $sp, $sp, 272
 0x11F: jal     string __global__::string::operator +($string,$data,string)
 0x120: subui   $sp, $sp, 272
 0x121: addui   $v3, $zero, 2834382347
 0x122: mptr    $s7, 0x44
 0x123: addui   $v0, $sp, 272
 0x124: st64    $v0, 0($sp)
 0x125: ld64    $a0, 0($sp)
 0x126: addu    $a1, $zero, $s7
 0x127: addui   $a2, $zero, 1
 0x128: addui   $sp, $sp, 296
 0x129: jal     void __global__::string::constructor($string,data,u32)
 0x12A: subui   $sp, $sp, 296
 0x12B: addui   $s7, $sp, 296
 0x12C: addu    $a0, $zero, $s5
 0x12D: addu    $a1, $zero, $s7
 0x12E: ld64    $a2, 0($sp)
 0x12F: addui   $sp, $sp, 320
 0x130: jal     string __global__::string::operator +($string,$data,string)
 0x131: subui   $sp, $sp, 320
 0x132: addu    $a0, $zero, $s1
 0x133: addui   $sp, $sp, 320
 0x134: jal     void __global__::string::destructor($string)
 0x135: subui   $sp, $sp, 320
 0x136: addu    $a0, $zero, $s2
 0x137: addui   $sp, $sp, 320
 0x138: jal     void __global__::string::destructor($string)
 0x139: subui   $sp, $sp, 320
 0x13A: addu    $a0, $zero, $s0
 0x13B: addui   $sp, $sp, 320
 0x13C: jal     void __global__::string::destructor($string)
 0x13D: subui   $sp, $sp, 320
 0x13E: addu    $a0, $zero, $s4
 0x13F: addui   $sp, $sp, 320
 0x140: jal     void __global__::string::destructor($string)
 0x141: subui   $sp, $sp, 320
 0x142: addu    $a0, $zero, $s3
 0x143: addui   $sp, $sp, 320
 0x144: jal     void __global__::string::destructor($string)
 0x145: subui   $sp, $sp, 320
 0x146: addu    $a0, $zero, $s6
 0x147: addui   $sp, $sp, 320
 0x148: jal     void __global__::string::destructor($string)
 0x149: subui   $sp, $sp, 320
 0x14A: addu    $a0, $zero, $s5
 0x14B: addui   $sp, $sp, 320
 0x14C: jal     void __global__::string::destructor($string)
 0x14D: subui   $sp, $sp, 320
 0x14E: ld64    $a0, 0($sp)
 0x14F: addui   $sp, $sp, 320
 0x150: jal     void __global__::string::destructor($string)
 0x151: subui   $sp, $sp, 320
 0x152: ld64    $v0, 0($sp)
 0x153: addu    $a0, $zero, $s7
 0x154: addui   $sp, $sp, 320
 0x155: jal     u32 __global__::print(string)
 0x156: subui   $sp, $sp, 320
 0x157: addu    $s5, $v0, $zero
 0x158: addu    $a0, $zero, $s7
 0x159: addui   $sp, $sp, 320
 0x15A: jal     void __global__::string::destructor($string)
 0x15B: subui   $sp, $sp, 320
 0x15C: ld64    $ra, 8($sp)
 0x15D: ld64    $a1, 16($sp)
 0x15E: ld64    $s3, 24($sp)
 0x15F: ld64    $s5, 32($sp)
 0x160: ld64    $f0, 40($sp)            ;       void print() {
 0x161: ld64    $a2, 48($sp)
 0x162: ld64    $s1, 56($sp)
 0x163: ld64    $s4, 64($sp)
 0x164: ld64    $s6, 72($sp)
 0x165: ld64    $s7, 80($sp)
 0x166: ld64    $s2, 88($sp)
 0x167: ld64    $s0, 96($sp)
 0x168: jmpr    $ra
```

#### With the following host code
```
i32 testCb (callback<i32(*)(i32, i32)> cb) {
    i32 x = cb(1, 2);
    return x;
}

int main(int arg_count, const char** args) {
    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    ctx.bind(testCb, "testCb");

    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    print_code(&be);
    
    mod->init();
}
```

#### Producing the following output
```
3.00
vec2f(2.00, 2.00)
vec2f(0.00, 0.00)
vec2f(1.00, 1.00)
vec2f(2.00, 2.00)
vec2f(3.00, 3.00)
vec2f(4.00, 4.00)
vec2f(5.00, 5.00)
vec2f(6.00, 6.00)
vec2f(7.00, 7.00)
vec2f(8.00, 8.00)
vec2f(9.00, 9.00)
```

Pretty neat

</details>
