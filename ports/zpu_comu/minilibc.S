// ZPU minilibc. Just enough to test without fear of LGPL retribution.
// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.
// Note that this license still holds for this file,
//  despite being part of the minetest-computech repository.

// Must be first in compilation order to work.
// (This is unreliable, but
//  not as bad as the
//   "custom section, which the compiler mysteriously
//    prefers, except when it's using a linker script"
//  method preferred by their libgloss...
//  Which, BTW, apparently doesn't apply on -nostdlib. Why? IDK.)

.globl _memreg
.globl _start
.globl _memory_end

.globl strlen
.globl memchr
.globl _memcpy_fast4

.globl digiline_msgc
.globl digiline_recv
.globl digiline_send

.section .text

// Ought to be <= 8 bytes.
_start:
impcrel .prebank
poppcrel
// @6
.balign 4,0
// @8
_digiline_io:
.long 0
_memory_end:
.long 0
// @16
_memreg:
.long 0
.long 0
.long 0
.long 0
// @32

.balign 32,0
// Interrupt Handler May Go Here
_interrupt:
poppc

// Computech-specific code.
// First order of business is to find the Digiline IO peripheral.
// This code is hopefully well-optimized for speed.
// (note: the code's down at .prebank, it's ordered to reduce instruction count)
.prebank_next:
loadsp 0
im 1
add
impcrel .prebank_term
eqbranch
// paddr
loadsp 0
loadh
// First word is peripheral length.
// plen paddr
add
impcrel .prebank_sd
poppcrel
.prebank_term:
storesp 0 // note: this is a "pop"
breakpoint // wait for interrupt, then try again
// Note that usually the first device is the Digiline IO Device,
//  so the best thing that can be done is to optimize for this case.
// If that is the case, it will not branch during this sequence.
.prebank:
pushsp
im _memory_end
store
im 0x80000000
.prebank_sd:
// paddr
loadsp 0
load
// [pdata] paddr
// Check for the precise signature of a Digiline IO device
im -(0x00100001)
add
impcrel .prebank_next
neqbranch
// paddr
im 12
add
im _digiline_io
store
// Start program.
impcrel main
callpcrel
_exit:
breakpoint
im _exit
poppc
// -- end prebank

strlen:
// retaddr strptr
loadsp 4
im 0
.strlen_loop:
// len strptr ...
loadsp 4
loadb
impcrel .strlen_end
eqbranch
// len strptr ...
loadsp 4
im 1
add
// strptr+1 len strptr ...
storesp 8
im 1
add
// len+1 strptr+1 ...
impcrel .strlen_loop
poppcrel
.strlen_end:
// len strptr ...
im _memreg+0
store
// strptr retaddr ostrptr
storesp 0
// retaddr ostrptr
poppc

memchr:
// ret str chr len
loadsp 12
// len ret str chr len
loadsp 0
// len len ret str chr len
impcrel .memchr_fail
eqbranch
// len ret str chr len
im -1
add
// len-1 ret str chr len
storesp 16
// ret str chr len-1
loadsp 4
loadsp 0
// str str ret str chr len-1
im 1
add
// str+1 str ret str chr len-1
storesp 12
// str ret str+1 chr len-1
loadb
// [str] ret str+1 chr len-1
loadsp 12
im 0xFF
and
xor
// [str]^chr ret str+1 chr len-1
impcrel memchr
neqbranch
// ret str+1 chr len-1
loadsp 4
im -1
add
im _memreg+0
store
poppc
.memchr_fail:
// 0 ret str chr 0
im _memreg+0
store
poppc

_memcpy_fast4:
// retaddr s1 s2 n
loadsp 8
// s2 retaddr s1 s2 n
loadsp 8
// s1 s2 retaddr s1 s2 n
loadsp 4
load
loadsp 4
// s1 [s2] s1 s2 retaddr s1 s2 n
store
// s1 s2 retaddr s1 s2 n
im 4
add
// s1+4 s2 retaddr s1 s2 n
storesp 12
// s2 retaddr s1 s2 n
im 4
add
// s2+4 retaddr s1 s2 n
storesp 12
// retaddr s1 s2 n
loadsp 12
im -4
add
// n-4 retaddr s1 s2 n
loadsp 0
// n-4 n-4 retaddr s1 s2 n
storesp 20

// n-4 retaddr s1 s2 n
impcrel _memcpy_fast4
neqbranch
poppc

// not much to say about this!
digiline_msgc:
im _digiline_io
load
load
im 0xFF
and
im _memreg+0
store
poppc


digiline_recv:
im 0x200
nop
impcrel .digiline_gf
poppcrel
// digiline_send(c*,int,c*,int)
digiline_send:
im 0x100
nop
// Most of both Digiline IO functions: making the actual call.
// Note these functions don't do length size checks,
//  as that's up to the user (which presumably has some plan)
.digiline_gf:
im _digiline_io
load
im -8
add
//                     1  1  2  2
// Cp[DIO]4wsig8retaddr2Ca6Cs0Da4Ds
loadsp 12
// Ca Cp[DIO] ...
loadsp 4
store
//                     1  1  2  2
// Cp[DIO]4wsig8retaddr2Ca6Cs0Da4Ds
im 4
add
//                     1  1  2  2
// Dp[DIO]4wsig8retaddr2Ca6Cs0Da4Ds
loadsp 20
// Da Dp[DIO] ...
loadsp 4
// Dp[DIO] Da Dp[DIO] ...
store
// Dp[DIO] ...
storesp 0
// wsig retaddr Ca Cs Da Ds
loadsp 12
im 24
ashiftleft
or
// wsig retaddr Ca Cs Da Ds
loadsp 20
im 16
ashiftleft
or
// wsig retaddr Ca Cs Da Ds
im _digiline_io
load
// cc[DIO] wsig retaddr Ca Cs Da Ds
loadsp 4
loadsp 4
// [DIO] wsig [DIO] wsig retaddr Ca Cs Da Ds
store
// [DIO] wsig retaddr Ca Cs Da Ds
load
// [[DIO]] wsig retaddr Ca Cs Da Ds
storesp 4
// [[DIO]] retaddr Ca Cs Da Ds
im _memreg+0
store
poppc
