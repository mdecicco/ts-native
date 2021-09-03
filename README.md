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
(pls ignore the very large amount of register backup & restoration instructions... I'll replace the mechanism that does that with proper function prologues and epilogues soon)
```
[void test::__init__() -> null]
 0x001: addui   $v3, $zero, 2834382347  ; import { vec2f } from 'math';
 0x002: mptr    $s0, 0x0
 0x003: addui   $s1, $zero, 0           ; i32()(i32, i32) cb = (i32 a, i32 b) : i32 => {
 0x004: st64    $s0, 0($sp)
 0x005: addui   $a0, $zero, 3453803796
 0x006: addu    $a1, $zero, $s1
 0x007: addui   $a2, $zero, 0
 0x008: st64    $ra, 8($sp)
 0x009: addui   $sp, $sp, 16
 0x00A: jal     data __global__::$makefunc(u32,data,u64)
 0x00B: subui   $sp, $sp, 16
 0x00C: addu    $s1, $v0, $zero
 0x00D: ld64    $ra, 8($sp)
 0x00E: ld64    $s0, 0($sp)
 0x00F: addu    $s2, $s1, $zero
 0x010: addu    $s1, $s2, $zero         ; import { vec2f } from 'math';
 0x011: st64    $s1, 0($s0)
 0x012: addui   $v3, $zero, 2834382347
 0x013: mptr    $s0, 0x8
 0x014: st64    $s0, 0($sp)             ; i32 x = testCb(cb);
 0x015: addu    $a0, $zero, $s1
 0x016: st64    $ra, 8($sp)
 0x017: addui   $sp, $sp, 16
 0x018: jal     i32 __global__::testCb(i32(i32,i32))
 0x019: subui   $sp, $sp, 16
 0x01A: addu    $s1, $v0, $zero
 0x01B: ld64    $ra, 8($sp)
 0x01C: ld64    $s0, 0($sp)
 0x01D: add     $s2, $s1, $zero         ; import { vec2f } from 'math';
 0x01E: st32    $s2, 0($s0)
 0x01F: addui   $s0, $sp, 0             ; print(x.toFixed(2));
 0x020: st64    $s0, 24($sp)
 0x021: addu    $a0, $zero, $s0
 0x022: addu    $a1, $zero, $s2
 0x023: addui   $a2, $zero, 2
 0x024: st64    $ra, 32($sp)
 0x025: addui   $sp, $sp, 40
 0x026: jal     string __global__::i32::toFixed($data,i32,u8)
 0x027: subui   $sp, $sp, 40
 0x028: ld64    $ra, 32($sp)
 0x029: ld64    $s0, 24($sp)
 0x02A: st64    $s0, 24($sp)
 0x02B: addu    $a0, $zero, $s0
 0x02C: st64    $ra, 32($sp)
 0x02D: addui   $sp, $sp, 40
 0x02E: jal     u32 __global__::print(string)
 0x02F: subui   $sp, $sp, 40
 0x030: addu    $s2, $v0, $zero
 0x031: ld64    $ra, 32($sp)
 0x032: ld64    $s0, 24($sp)
 0x033: st64    $s0, 24($sp)
 0x034: addu    $a0, $zero, $s0
 0x035: st64    $ra, 32($sp)
 0x036: addui   $sp, $sp, 40
 0x037: jal     void __global__::string::destructor($string)
 0x038: subui   $sp, $sp, 40
 0x039: ld64    $ra, 32($sp)
 0x03A: ld64    $s0, 24($sp)
 0x03B: addui   $s0, $sp, 0             ; some_guy a = some_guy(2, 2);
 0x03C: st64    $s0, 8($sp)
 0x03D: addu    $a0, $zero, $s0
 0x03E: faddi   $f0, $zero, 2.000000
 0x03F: faddi   $f1, $zero, 2.000000
 0x040: st64    $ra, 16($sp)
 0x041: addui   $sp, $sp, 24
 0x042: jal     void test::some_guy::constructor($some_guy,f32,f32)
 0x043: subui   $sp, $sp, 24
 0x044: ld64    $ra, 16($sp)
 0x045: ld64    $s0, 8($sp)
 0x046: addui   $v3, $zero, 2834382347
 0x047: mptr    $s2, 0xC
 0x048: st64    $s0, 8($sp)
 0x049: st64    $s2, 16($sp)
 0x04A: addu    $a0, $zero, $s2
 0x04B: addu    $a1, $zero, $s0
 0x04C: st64    $ra, 24($sp)
 0x04D: addui   $sp, $sp, 32
 0x04E: jal     void test::some_guy::constructor($some_guy,some_guy)
 0x04F: subui   $sp, $sp, 32
 0x050: ld64    $ra, 24($sp)
 0x051: ld64    $s0, 8($sp)
 0x052: ld64    $s2, 16($sp)
 0x053: st64    $s0, 8($sp)             ; a.print();
 0x054: addu    $a0, $zero, $s2
 0x055: st64    $ra, 16($sp)
 0x056: addui   $sp, $sp, 24
 0x057: jal     void test::some_guy::print($some_guy)
 0x058: subui   $sp, $sp, 24
 0x059: ld64    $ra, 16($sp)
 0x05A: ld64    $s0, 8($sp)
 0x05B: addui   $v3, $zero, 2834382347  ; array<some_guy> arr;
 0x05C: mptr    $s2, 0x14
 0x05D: addui   $v3, $zero, -6273164584710744737
 0x05E: st64    $s0, 8($sp)
 0x05F: st64    $s2, 16($sp)
 0x060: addu    $a0, $zero, $s2
 0x061: addui   $a1, $zero, -6273164584710744737
 0x062: st64    $ra, 24($sp)
 0x063: addui   $sp, $sp, 32
 0x064: jal     void __global__::array::constructor($array,$u64)
 0x065: subui   $sp, $sp, 32
 0x066: ld64    $ra, 24($sp)
 0x067: ld64    $s0, 8($sp)
 0x068: ld64    $s2, 16($sp)
 0x069: addui   $v3, $zero, 2834382347  ; for (u32 i = 0;i < 10;i++) {
 0x06A: mptr    $s1, 0x34
 0x06B: addui   $s3, $zero, 0
 0x06C: st32    $s3, 0($s1)
 0x06D: lti     $s4, $s3, 10
 0x06E: bneqz   $s4, 0x9D
 0x06F: addui   $s4, $sp, 8             ;       arr.push(some_guy(i, i));
 0x070: mtfp    $s3, $f0
 0x071: cvt.u.f $f0
 0x072: mtfp    $s3, $f0
 0x073: cvt.u.f $f0
 0x074: st64    $s0, 16($sp)
 0x075: st64    $s2, 24($sp)
 0x076: st64    $s1, 32($sp)
 0x077: st32    $s3, 40($sp)
 0x078: st64    $s4, 44($sp)
 0x079: addu    $a0, $zero, $s4
 0x07A: fadd    $f1, $zero, $f0
 0x07B: fadd    $f0, $zero, $f0
 0x07C: st64    $ra, 52($sp)
 0x07D: addui   $sp, $sp, 60
 0x07E: jal     void test::some_guy::constructor($some_guy,f32,f32)
 0x07F: subui   $sp, $sp, 60
 0x080: ld64    $ra, 52($sp)
 0x081: ld64    $s0, 16($sp)
 0x082: ld64    $s2, 24($sp)
 0x083: ld64    $s1, 32($sp)
 0x084: ld32    $s3, 40($sp)
 0x085: ld64    $s4, 44($sp)
 0x086: addui   $v3, $zero, -6273164584710744737
 0x087: st64    $s0, 16($sp)
 0x088: st64    $s2, 24($sp)
 0x089: st64    $s1, 32($sp)
 0x08A: st32    $s3, 40($sp)
 0x08B: st64    $s4, 44($sp)
 0x08C: addu    $a0, $zero, $s2
 0x08D: addu    $a1, $zero, $s4
 0x08E: st64    $ra, 52($sp)
 0x08F: addui   $sp, $sp, 60
 0x090: jal     void __global__::array::push($array,subtype)
 0x091: subui   $sp, $sp, 60
 0x092: ld64    $ra, 52($sp)
 0x093: ld64    $s0, 16($sp)
 0x094: ld64    $s2, 24($sp)
 0x095: ld64    $s1, 32($sp)
 0x096: ld32    $s3, 40($sp)
 0x097: ld64    $s4, 44($sp)
 0x098: addu    $s4, $s3, $zero         ; for (u32 i = 0;i < 10;i++) {
 0x099: addui   $s4, $s3, 1
 0x09A: addu    $s3, $s4, $zero
 0x09B: st32    $s3, 0($s1)
 0x09C: jmp     0x6D
 0x09D: addui   $v3, $zero, 2834382347  ; for (u32 i = 0;i < arr.length;i++) arr[i].print();
 0x09E: mptr    $s3, 0x38
 0x09F: addui   $s1, $zero, 0
 0x0A0: st32    $s1, 0($s3)
 0x0A1: addui   $v3, $zero, -6273164584710744737
 0x0A2: st64    $s0, 8($sp)
 0x0A3: st64    $s2, 16($sp)
 0x0A4: st64    $s3, 24($sp)
 0x0A5: st32    $s1, 32($sp)
 0x0A6: addu    $a0, $zero, $s2
 0x0A7: st64    $ra, 36($sp)
 0x0A8: addui   $sp, $sp, 44
 0x0A9: jal     u32 __global__::array::get_length($array)
 0x0AA: subui   $sp, $sp, 44
 0x0AB: addu    $s4, $v0, $zero
 0x0AC: ld64    $ra, 36($sp)
 0x0AD: ld64    $s0, 8($sp)
 0x0AE: ld64    $s2, 16($sp)
 0x0AF: ld64    $s3, 24($sp)
 0x0B0: ld32    $s1, 32($sp)
 0x0B1: lt      $s5, $s1, $s4
 0x0B2: bneqz   $s5, 0xD7
 0x0B3: addui   $v3, $zero, -6273164584710744737
 0x0B4: st64    $s0, 8($sp)
 0x0B5: st64    $s2, 16($sp)
 0x0B6: st64    $s3, 24($sp)
 0x0B7: st32    $s1, 32($sp)
 0x0B8: addu    $a0, $zero, $s2
 0x0B9: addu    $a1, $zero, $s1
 0x0BA: st64    $ra, 36($sp)
 0x0BB: addui   $sp, $sp, 44
 0x0BC: jal     subtype __global__::array::operator []($array,u32)
 0x0BD: subui   $sp, $sp, 44
 0x0BE: addu    $s5, $v0, $zero
 0x0BF: ld64    $ra, 36($sp)
 0x0C0: ld64    $s0, 8($sp)
 0x0C1: ld64    $s2, 16($sp)
 0x0C2: ld64    $s3, 24($sp)
 0x0C3: ld32    $s1, 32($sp)
 0x0C4: st64    $s0, 8($sp)
 0x0C5: st64    $s2, 16($sp)
 0x0C6: st64    $s3, 24($sp)
 0x0C7: st32    $s1, 32($sp)
 0x0C8: addu    $a0, $zero, $s5
 0x0C9: st64    $ra, 36($sp)
 0x0CA: addui   $sp, $sp, 44
 0x0CB: jal     void test::some_guy::print($some_guy)
 0x0CC: subui   $sp, $sp, 44
 0x0CD: ld64    $ra, 36($sp)
 0x0CE: ld64    $s0, 8($sp)
 0x0CF: ld64    $s2, 16($sp)
 0x0D0: ld64    $s3, 24($sp)
 0x0D1: ld32    $s1, 32($sp)
 0x0D2: addu    $s5, $s1, $zero
 0x0D3: addui   $s5, $s1, 1
 0x0D4: addu    $s1, $s5, $zero
 0x0D5: st32    $s1, 0($s3)
 0x0D6: jmp     0xA1
 0x0D7: jmpr    $ra                     ; import { vec2f } from 'math';

[i32 test::lambda_0(i32 arg_0 -> $a0, i32 arg_1 -> $a1) -> $v0]
 0x0D8: add     $s0, $a0, $a1           ;       return a + b;
 0x0D9: addu    $v0, $s0, $zero
 0x0DA: jmpr    $ra

[void test::some_guy::constructor(some_guy @this -> $a0, f32 arg_1 -> $f0, f32 arg_2 -> $f1) -> null]
 0x0DB: addui   $s0, $a0, 0             ;       constructor(f32 x, f32 y) : v(vec2f(x, y)) { }
 0x0DC: addui   $s1, $sp, 0
 0x0DD: st64    $s0, 8($sp)
 0x0DE: st64    $s1, 16($sp)
 0x0DF: st64    $a0, 24($sp)
 0x0E0: st32    $f0, 32($sp)
 0x0E1: st32    $f1, 36($sp)
 0x0E2: addu    $a0, $zero, $s1
 0x0E3: ld32    $f0, 32($sp)
 0x0E4: ld32    $f1, 36($sp)
 0x0E5: st64    $ra, 40($sp)
 0x0E6: addui   $sp, $sp, 48
 0x0E7: jal     void math::vec2f::constructor($vec2f,f32,f32)
 0x0E8: subui   $sp, $sp, 48
 0x0E9: ld64    $ra, 40($sp)
 0x0EA: ld64    $s0, 8($sp)
 0x0EB: ld64    $s1, 16($sp)
 0x0EC: ld64    $a0, 24($sp)
 0x0ED: ld32    $f0, 32($sp)
 0x0EE: ld32    $f1, 36($sp)
 0x0EF: st64    $s1, 8($sp)
 0x0F0: st64    $a0, 16($sp)
 0x0F1: addu    $a0, $zero, $s0
 0x0F2: addu    $a1, $zero, $s1
 0x0F3: st64    $ra, 24($sp)
 0x0F4: addui   $sp, $sp, 32
 0x0F5: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x0F6: subui   $sp, $sp, 32
 0x0F7: ld64    $ra, 24($sp)
 0x0F8: ld64    $s1, 8($sp)
 0x0F9: ld64    $a0, 16($sp)
 0x0FA: st64    $s1, 8($sp)
 0x0FB: st64    $a0, 16($sp)
 0x0FC: addu    $a0, $zero, $s1
 0x0FD: st64    $ra, 24($sp)
 0x0FE: addui   $sp, $sp, 32
 0x0FF: jal     void math::vec2f::destructor($vec2f)
 0x100: subui   $sp, $sp, 32
 0x101: ld64    $ra, 24($sp)
 0x102: ld64    $s1, 8($sp)
 0x103: ld64    $a0, 16($sp)
 0x104: jmpr    $ra

[void test::some_guy::constructor(some_guy @this -> $a0, some_guy arg_1 -> $a1) -> null]
 0x105: addui   $s0, $a0, 0             ;       constructor(some_guy g) : v(vec2f(g.v)) { }
 0x106: addui   $s1, $a1, 0
 0x107: addu    $s2, $s1, $zero
 0x108: addui   $s1, $sp, 0
 0x109: st64    $s0, 8($sp)
 0x10A: st64    $s1, 16($sp)
 0x10B: st64    $a0, 24($sp)
 0x10C: st64    $a1, 32($sp)
 0x10D: addu    $a0, $zero, $s1
 0x10E: addu    $a1, $zero, $s2
 0x10F: st64    $ra, 40($sp)
 0x110: addui   $sp, $sp, 48
 0x111: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x112: subui   $sp, $sp, 48
 0x113: ld64    $ra, 40($sp)
 0x114: ld64    $s0, 8($sp)
 0x115: ld64    $s1, 16($sp)
 0x116: ld64    $a0, 24($sp)
 0x117: ld64    $a1, 32($sp)
 0x118: st64    $s1, 8($sp)
 0x119: st64    $a0, 16($sp)
 0x11A: st64    $a1, 24($sp)
 0x11B: addu    $a0, $zero, $s0
 0x11C: addu    $a1, $zero, $s1
 0x11D: st64    $ra, 32($sp)
 0x11E: addui   $sp, $sp, 40
 0x11F: jal     void math::vec2f::constructor($vec2f,vec2f)
 0x120: subui   $sp, $sp, 40
 0x121: ld64    $ra, 32($sp)
 0x122: ld64    $s1, 8($sp)
 0x123: ld64    $a0, 16($sp)
 0x124: ld64    $a1, 24($sp)
 0x125: st64    $s1, 8($sp)
 0x126: st64    $a0, 16($sp)
 0x127: addu    $a0, $zero, $s1
 0x128: st64    $ra, 24($sp)
 0x129: addui   $sp, $sp, 32
 0x12A: jal     void math::vec2f::destructor($vec2f)
 0x12B: subui   $sp, $sp, 32
 0x12C: ld64    $ra, 24($sp)
 0x12D: ld64    $s1, 8($sp)
 0x12E: ld64    $a0, 16($sp)
 0x12F: jmpr    $ra

[void test::some_guy::print(some_guy @this -> $a0) -> null]
 0x130: addui   $v3, $zero, 2834382347  ;               print('vec2f(' + this.v.x.toFixed(2) + ', ' + this.v.y.toFixed(2) + ')');
 0x131: mptr    $s0, 0x3C
 0x132: addui   $s1, $sp, 8
 0x133: st64    $s1, 32($sp)
 0x134: st64    $a0, 40($sp)
 0x135: addu    $a0, $zero, $s1
 0x136: addu    $a1, $zero, $s0
 0x137: addui   $a2, $zero, 6
 0x138: st64    $ra, 48($sp)
 0x139: addui   $sp, $sp, 56
 0x13A: jal     void __global__::string::constructor($string,data,u32)
 0x13B: subui   $sp, $sp, 56
 0x13C: ld64    $ra, 48($sp)
 0x13D: ld64    $s1, 32($sp)
 0x13E: ld64    $a0, 40($sp)
 0x13F: addui   $s0, $a0, 0
 0x140: addu    $s2, $s0, $zero
 0x141: addui   $s0, $s2, 0
 0x142: ld32    $f0, 0($s2)
 0x143: addui   $s2, $sp, 32
 0x144: st64    $s1, 56($sp)
 0x145: st64    $s2, 64($sp)
 0x146: st64    $a0, 72($sp)
 0x147: addu    $a0, $zero, $s2
 0x148: fadd    $f0, $zero, $f0
 0x149: addui   $a1, $zero, 2
 0x14A: st64    $ra, 80($sp)
 0x14B: addui   $sp, $sp, 88
 0x14C: jal     string __global__::f32::toFixed($data,f32,u8)
 0x14D: subui   $sp, $sp, 88
 0x14E: ld64    $ra, 80($sp)
 0x14F: ld64    $s1, 56($sp)
 0x150: ld64    $s2, 64($sp)
 0x151: ld64    $a0, 72($sp)
 0x152: addui   $s0, $sp, 56
 0x153: st64    $s1, 80($sp)
 0x154: st64    $s2, 88($sp)
 0x155: st64    $s0, 96($sp)
 0x156: st64    $a0, 104($sp)
 0x157: addu    $a0, $zero, $s1
 0x158: addu    $a1, $zero, $s0
 0x159: addu    $a2, $zero, $s2
 0x15A: st64    $ra, 112($sp)
 0x15B: addui   $sp, $sp, 120
 0x15C: jal     string __global__::string::operator +($string,$data,string)
 0x15D: subui   $sp, $sp, 120
 0x15E: ld64    $ra, 112($sp)
 0x15F: ld64    $s1, 80($sp)
 0x160: ld64    $s2, 88($sp)
 0x161: ld64    $s0, 96($sp)
 0x162: ld64    $a0, 104($sp)
 0x163: addui   $v3, $zero, 2834382347
 0x164: mptr    $s3, 0x42
 0x165: addui   $s4, $sp, 80
 0x166: st64    $s1, 104($sp)
 0x167: st64    $s2, 112($sp)
 0x168: st64    $s0, 120($sp)
 0x169: st64    $s4, 128($sp)
 0x16A: st64    $a0, 136($sp)
 0x16B: addu    $a0, $zero, $s4
 0x16C: addu    $a1, $zero, $s3
 0x16D: addui   $a2, $zero, 2
 0x16E: st64    $ra, 144($sp)
 0x16F: addui   $sp, $sp, 152
 0x170: jal     void __global__::string::constructor($string,data,u32)
 0x171: subui   $sp, $sp, 152
 0x172: ld64    $ra, 144($sp)
 0x173: ld64    $s1, 104($sp)
 0x174: ld64    $s2, 112($sp)
 0x175: ld64    $s0, 120($sp)
 0x176: ld64    $s4, 128($sp)
 0x177: ld64    $a0, 136($sp)
 0x178: addui   $s3, $sp, 104
 0x179: st64    $s1, 128($sp)
 0x17A: st64    $s2, 136($sp)
 0x17B: st64    $s0, 144($sp)
 0x17C: st64    $s4, 152($sp)
 0x17D: st64    $s3, 160($sp)
 0x17E: st64    $a0, 168($sp)
 0x17F: addu    $a0, $zero, $s0
 0x180: addu    $a1, $zero, $s3
 0x181: addu    $a2, $zero, $s4
 0x182: st64    $ra, 176($sp)
 0x183: addui   $sp, $sp, 184
 0x184: jal     string __global__::string::operator +($string,$data,string)
 0x185: subui   $sp, $sp, 184
 0x186: ld64    $ra, 176($sp)
 0x187: ld64    $s1, 128($sp)
 0x188: ld64    $s2, 136($sp)
 0x189: ld64    $s0, 144($sp)
 0x18A: ld64    $s4, 152($sp)
 0x18B: ld64    $s3, 160($sp)
 0x18C: ld64    $a0, 168($sp)
 0x18D: addui   $s5, $a0, 0
 0x18E: addu    $s6, $s5, $zero
 0x18F: addui   $s5, $s6, 4
 0x190: ld32    $f0, 4($s6)
 0x191: addui   $s6, $sp, 128
 0x192: st64    $s1, 152($sp)
 0x193: st64    $s2, 160($sp)
 0x194: st64    $s0, 168($sp)
 0x195: st64    $s4, 176($sp)
 0x196: st64    $s3, 184($sp)
 0x197: st64    $s6, 192($sp)
 0x198: st64    $a0, 200($sp)
 0x199: addu    $a0, $zero, $s6
 0x19A: fadd    $f0, $zero, $f0
 0x19B: addui   $a1, $zero, 2
 0x19C: st64    $ra, 208($sp)
 0x19D: addui   $sp, $sp, 216
 0x19E: jal     string __global__::f32::toFixed($data,f32,u8)
 0x19F: subui   $sp, $sp, 216
 0x1A0: ld64    $ra, 208($sp)
 0x1A1: ld64    $s1, 152($sp)
 0x1A2: ld64    $s2, 160($sp)
 0x1A3: ld64    $s0, 168($sp)
 0x1A4: ld64    $s4, 176($sp)
 0x1A5: ld64    $s3, 184($sp)
 0x1A6: ld64    $s6, 192($sp)
 0x1A7: ld64    $a0, 200($sp)
 0x1A8: addui   $s5, $sp, 152
 0x1A9: st64    $s1, 176($sp)
 0x1AA: st64    $s2, 184($sp)
 0x1AB: st64    $s0, 192($sp)
 0x1AC: st64    $s4, 200($sp)
 0x1AD: st64    $s3, 208($sp)
 0x1AE: st64    $s6, 216($sp)
 0x1AF: st64    $s5, 224($sp)
 0x1B0: st64    $a0, 232($sp)
 0x1B1: addu    $a0, $zero, $s3
 0x1B2: addu    $a1, $zero, $s5
 0x1B3: addu    $a2, $zero, $s6
 0x1B4: st64    $ra, 240($sp)
 0x1B5: addui   $sp, $sp, 248
 0x1B6: jal     string __global__::string::operator +($string,$data,string)
 0x1B7: subui   $sp, $sp, 248
 0x1B8: ld64    $ra, 240($sp)
 0x1B9: ld64    $s1, 176($sp)
 0x1BA: ld64    $s2, 184($sp)
 0x1BB: ld64    $s0, 192($sp)
 0x1BC: ld64    $s4, 200($sp)
 0x1BD: ld64    $s3, 208($sp)
 0x1BE: ld64    $s6, 216($sp)
 0x1BF: ld64    $s5, 224($sp)
 0x1C0: ld64    $a0, 232($sp)
 0x1C1: addui   $v3, $zero, 2834382347
 0x1C2: mptr    $s7, 0x44
 0x1C3: addui   $v0, $sp, 176
 0x1C4: st64    $v0, 0($sp)
 0x1C5: st64    $s1, 200($sp)
 0x1C6: st64    $s2, 208($sp)
 0x1C7: st64    $s0, 216($sp)
 0x1C8: st64    $s4, 224($sp)
 0x1C9: st64    $s3, 232($sp)
 0x1CA: st64    $s6, 240($sp)
 0x1CB: st64    $s5, 248($sp)
 0x1CC: st64    $a0, 256($sp)
 0x1CD: ld64    $a0, 0($sp)
 0x1CE: addu    $a1, $zero, $s7
 0x1CF: addui   $a2, $zero, 1
 0x1D0: st64    $ra, 264($sp)
 0x1D1: addui   $sp, $sp, 272
 0x1D2: jal     void __global__::string::constructor($string,data,u32)
 0x1D3: subui   $sp, $sp, 272
 0x1D4: ld64    $ra, 264($sp)
 0x1D5: ld64    $s1, 200($sp)
 0x1D6: ld64    $s2, 208($sp)
 0x1D7: ld64    $s0, 216($sp)
 0x1D8: ld64    $s4, 224($sp)
 0x1D9: ld64    $s3, 232($sp)
 0x1DA: ld64    $s6, 240($sp)
 0x1DB: ld64    $s5, 248($sp)
 0x1DC: ld64    $a0, 256($sp)
 0x1DD: addui   $s7, $sp, 200
 0x1DE: st64    $s1, 224($sp)
 0x1DF: st64    $s2, 232($sp)
 0x1E0: st64    $s0, 240($sp)
 0x1E1: st64    $s4, 248($sp)
 0x1E2: st64    $s3, 256($sp)
 0x1E3: st64    $s6, 264($sp)
 0x1E4: st64    $s5, 272($sp)
 0x1E5: st64    $s7, 280($sp)
 0x1E6: st64    $a0, 288($sp)
 0x1E7: addu    $a0, $zero, $s5
 0x1E8: addu    $a1, $zero, $s7
 0x1E9: ld64    $a2, 0($sp)
 0x1EA: st64    $ra, 296($sp)
 0x1EB: addui   $sp, $sp, 304
 0x1EC: jal     string __global__::string::operator +($string,$data,string)
 0x1ED: subui   $sp, $sp, 304
 0x1EE: ld64    $ra, 296($sp)
 0x1EF: ld64    $s1, 224($sp)
 0x1F0: ld64    $s2, 232($sp)
 0x1F1: ld64    $s0, 240($sp)
 0x1F2: ld64    $s4, 248($sp)
 0x1F3: ld64    $s3, 256($sp)
 0x1F4: ld64    $s6, 264($sp)
 0x1F5: ld64    $s5, 272($sp)
 0x1F6: ld64    $s7, 280($sp)
 0x1F7: ld64    $a0, 288($sp)
 0x1F8: st64    $s1, 224($sp)
 0x1F9: st64    $s2, 232($sp)
 0x1FA: st64    $s0, 240($sp)
 0x1FB: st64    $s4, 248($sp)
 0x1FC: st64    $s3, 256($sp)
 0x1FD: st64    $s6, 264($sp)
 0x1FE: st64    $s5, 272($sp)
 0x1FF: st64    $s7, 280($sp)
 0x200: st64    $a0, 288($sp)
 0x201: addu    $a0, $zero, $s1
 0x202: st64    $ra, 296($sp)
 0x203: addui   $sp, $sp, 304
 0x204: jal     void __global__::string::destructor($string)
 0x205: subui   $sp, $sp, 304
 0x206: ld64    $ra, 296($sp)
 0x207: ld64    $s1, 224($sp)
 0x208: ld64    $s2, 232($sp)
 0x209: ld64    $s0, 240($sp)
 0x20A: ld64    $s4, 248($sp)
 0x20B: ld64    $s3, 256($sp)
 0x20C: ld64    $s6, 264($sp)
 0x20D: ld64    $s5, 272($sp)
 0x20E: ld64    $s7, 280($sp)
 0x20F: ld64    $a0, 288($sp)
 0x210: st64    $s2, 8($sp)
 0x211: st64    $s0, 16($sp)
 0x212: st64    $s4, 24($sp)
 0x213: st64    $s3, 224($sp)
 0x214: st64    $s6, 232($sp)
 0x215: st64    $s5, 240($sp)
 0x216: st64    $s7, 248($sp)
 0x217: st64    $a0, 256($sp)
 0x218: addu    $a0, $zero, $s2
 0x219: st64    $ra, 264($sp)
 0x21A: addui   $sp, $sp, 272
 0x21B: jal     void __global__::string::destructor($string)
 0x21C: subui   $sp, $sp, 272
 0x21D: ld64    $ra, 264($sp)
 0x21E: ld64    $s2, 8($sp)
 0x21F: ld64    $s0, 16($sp)
 0x220: ld64    $s4, 24($sp)
 0x221: ld64    $s3, 224($sp)
 0x222: ld64    $s6, 232($sp)
 0x223: ld64    $s5, 240($sp)
 0x224: ld64    $s7, 248($sp)
 0x225: ld64    $a0, 256($sp)
 0x226: st64    $s0, 8($sp)
 0x227: st64    $s4, 16($sp)
 0x228: st64    $s3, 24($sp)
 0x229: st64    $s6, 32($sp)
 0x22A: st64    $s5, 40($sp)
 0x22B: st64    $s7, 48($sp)
 0x22C: st64    $a0, 224($sp)
 0x22D: addu    $a0, $zero, $s0
 0x22E: st64    $ra, 232($sp)
 0x22F: addui   $sp, $sp, 240
 0x230: jal     void __global__::string::destructor($string)
 0x231: subui   $sp, $sp, 240
 0x232: ld64    $ra, 232($sp)
 0x233: ld64    $s0, 8($sp)
 0x234: ld64    $s4, 16($sp)
 0x235: ld64    $s3, 24($sp)
 0x236: ld64    $s6, 32($sp)
 0x237: ld64    $s5, 40($sp)
 0x238: ld64    $s7, 48($sp)
 0x239: ld64    $a0, 224($sp)
 0x23A: st64    $s4, 8($sp)
 0x23B: st64    $s3, 16($sp)
 0x23C: st64    $s6, 24($sp)
 0x23D: st64    $s5, 32($sp)
 0x23E: st64    $s7, 40($sp)
 0x23F: st64    $a0, 48($sp)
 0x240: addu    $a0, $zero, $s4
 0x241: st64    $ra, 56($sp)
 0x242: addui   $sp, $sp, 224
 0x243: jal     void __global__::string::destructor($string)
 0x244: subui   $sp, $sp, 224
 0x245: ld64    $ra, 56($sp)
 0x246: ld64    $s4, 8($sp)
 0x247: ld64    $s3, 16($sp)
 0x248: ld64    $s6, 24($sp)
 0x249: ld64    $s5, 32($sp)
 0x24A: ld64    $s7, 40($sp)
 0x24B: ld64    $a0, 48($sp)
 0x24C: st64    $s3, 8($sp)
 0x24D: st64    $s6, 16($sp)
 0x24E: st64    $s5, 24($sp)
 0x24F: st64    $s7, 32($sp)
 0x250: st64    $a0, 40($sp)
 0x251: addu    $a0, $zero, $s3
 0x252: st64    $ra, 48($sp)
 0x253: addui   $sp, $sp, 224
 0x254: jal     void __global__::string::destructor($string)
 0x255: subui   $sp, $sp, 224
 0x256: ld64    $ra, 48($sp)
 0x257: ld64    $s3, 8($sp)
 0x258: ld64    $s6, 16($sp)
 0x259: ld64    $s5, 24($sp)
 0x25A: ld64    $s7, 32($sp)
 0x25B: ld64    $a0, 40($sp)
 0x25C: st64    $s6, 8($sp)
 0x25D: st64    $s5, 16($sp)
 0x25E: st64    $s7, 24($sp)
 0x25F: st64    $a0, 32($sp)
 0x260: addu    $a0, $zero, $s6
 0x261: st64    $ra, 40($sp)
 0x262: addui   $sp, $sp, 224
 0x263: jal     void __global__::string::destructor($string)
 0x264: subui   $sp, $sp, 224
 0x265: ld64    $ra, 40($sp)
 0x266: ld64    $s6, 8($sp)
 0x267: ld64    $s5, 16($sp)
 0x268: ld64    $s7, 24($sp)
 0x269: ld64    $a0, 32($sp)
 0x26A: st64    $s5, 8($sp)
 0x26B: st64    $s7, 16($sp)
 0x26C: st64    $a0, 24($sp)
 0x26D: addu    $a0, $zero, $s5
 0x26E: st64    $ra, 32($sp)
 0x26F: addui   $sp, $sp, 224
 0x270: jal     void __global__::string::destructor($string)
 0x271: subui   $sp, $sp, 224
 0x272: ld64    $ra, 32($sp)
 0x273: ld64    $s5, 8($sp)
 0x274: ld64    $s7, 16($sp)
 0x275: ld64    $a0, 24($sp)
 0x276: st64    $s7, 8($sp)
 0x277: st64    $a0, 16($sp)
 0x278: ld64    $a0, 0($sp)
 0x279: st64    $ra, 24($sp)
 0x27A: addui   $sp, $sp, 224
 0x27B: jal     void __global__::string::destructor($string)
 0x27C: subui   $sp, $sp, 224
 0x27D: ld64    $ra, 24($sp)
 0x27E: ld64    $s7, 8($sp)
 0x27F: ld64    $a0, 16($sp)
 0x280: ld64    $v0, 0($sp)
 0x281: st64    $s7, 8($sp)
 0x282: st64    $a0, 16($sp)
 0x283: addu    $a0, $zero, $s7
 0x284: st64    $ra, 24($sp)
 0x285: addui   $sp, $sp, 224
 0x286: jal     u32 __global__::print(string)
 0x287: subui   $sp, $sp, 224
 0x288: addu    $s5, $v0, $zero
 0x289: ld64    $ra, 24($sp)
 0x28A: ld64    $s7, 8($sp)
 0x28B: ld64    $a0, 16($sp)
 0x28C: st64    $s7, 8($sp)
 0x28D: st64    $a0, 16($sp)
 0x28E: addu    $a0, $zero, $s7
 0x28F: st64    $ra, 24($sp)
 0x290: addui   $sp, $sp, 224
 0x291: jal     void __global__::string::destructor($string)
 0x292: subui   $sp, $sp, 224
 0x293: ld64    $ra, 24($sp)
 0x294: ld64    $s7, 8($sp)
 0x295: ld64    $a0, 16($sp)
 0x296: jmpr    $ra                     ;       void print() {
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
