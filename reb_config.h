// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#ifndef REB_CONFIG_H

// The type used by REB_RES_NUMBER
#define REB_NUMTYPE int

/* a tiny primer on Recovery Basic IO:
 * Handles are numbers.
 * Referring to handles is actually referring to variables with handle-y names.
 * Some commands (will) have special parsing which lets this work:
 * input#fileHandle a$
 * You may ask "Why this way???".
 * Well, a global filetable plan was looking like it would be a mess to implement.
 * And named files is better anyway.
 */

// If you are insane enough to define this, the value == amount of memory. In bytes.
// BTW: 1536 is how much I estimate I'd be able to safely use from an Arduino.
// The memory used up by the Prompt brings down the *actual* amount to 1304,
//  before I made calls to reb_mem_available do a full block-merge.
// Now it's 1400. Suffice to say, the allocator isn't great.
// At least this 136-byte overhead's only on a 64-bit machine. Hopefully actual HW will be nicer.

// Additional note: There's tons of debugging in the custom allocator,
//                   which is probably it's only practical use.
//                  Keeps traces on what performed the allocation.
//                  To find out the full stack, just run it through twice:
//                  Once to find what addresses you're looking for,
//                   again (with the same build but under GDB.
//                          This is important, as this keeps the addresses deterministic)
//                  in order to actually find out callstack information.
//                  There's also a few sanity asserts in there too even with debug off.

#define REB_MEM_CONFIG_CUSTOMALLOCATOR 32768

// Platform-specific optimization to make reb_gets_internal a lot faster.
// Define if, and only if, the reb_io implementation in use will always end reads when it hits a '\n'.
// Undefine if this is not the case.
#define REB_IO_NEWLINE_ENDS_READ

// Useful if you want to reduce memory usage.
// Otherwise, don't bother.
//#define REB_NO_HELP

#endif
