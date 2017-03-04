// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

// ZPU minilibc. Just enough to test without fear of LGPL retribution.

// Must be first in compilation order to work.
// (This is unreliable, but
//  not as bad as the
//   "custom section, which the compiler mysteriously
//    prefers, except when it's using a linker script"M
//  method preferred by their libgloss...
//  Which, BTW, apparently doesn't apply on -nostdlib. Why? IDK.)

.globl _memreg
.globl _start
.globl inbyte
.globl outbyte
.globl _memory_end

.section .text

// Ought to be 8 bytes.
_start:
pushsp
im _memory_end
store
im _break
nop
im main
poppc
_break:
breakpoint

.balign 4,0
_memory_end:
.long 0
_memreg:
.long 0
.long 0
.long 0
.long 0

.balign 32,0
// Interrupt Handler May Go Here
_interrupt:
poppc

// inbyte leaves some stack about when branching
//  if it doesn't immediately succeed, clean it up here
.inbyte_stack:
storesp 0

inbyte:
im 0x80000028
// 0x8.28
load
loadsp 0
im 0x100
// 0x100 [0x8.28] [0x8.28]
and
impcrel .inbyte_stack
eqbranch
im 0xFF
and
// retVal retAddr
im _memreg+0
store
poppc

// The calling convention is confusing.
// _memreg+0 is DEFINITELY return value.
// Arguments are caller-cleanup.
// _memreg+12 is ebp (with offset -4, so [[_memreg+12]+8] is first arg).
// This is kind of complicated ^.^;
outbyte:
im 0x80000024
load
im 0x100
and
impcrel outbyte
eqbranch
// Here's the different part.
loadsp 4
im 0x80000024
store
poppc
