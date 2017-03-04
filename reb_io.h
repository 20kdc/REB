// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#ifndef REB_IO_H
#define REB_IO_H

// Just for size_t, really.
#include <stdlib.h>

#include "reb_config.h"

// On Unix-likes, REB_NUMTYPE being the FD is fine + a few (int) casts for safety.
// On Windows, this will have to be backed by some sort of emulation.
// On platforms with no file IO support, the answer is simple, ignore the problem.

#define REB_IO_CONSOLE 0
#define REB_IO_KEYBOARD 1

// Reads len bytes from handle into data.
// Returns 0 if all is okay, anything else means error.
// The amount of bytes actually read is put into lenres.
// It is possible to end up reading 0 bytes without error
//  (think EOF or nonblocking.)
// (Input length of 0 bytes should be successful, but don't count on it.)
int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * lenres);

// Similar to read in that error is nonzero.
// I'm not sure if anyone knows what to do on partial write, nevermind me.
// For now save_core assumes this always succeeds since I have no better idea.
int reb_io_write(REB_NUMTYPE handle, char * data, size_t len);

// Use solely for C-string messages (error reporting error due to console for reporting errors failing == just give up, really)
void reb_io_puts(REB_NUMTYPE handle, char * str);

#define REB_IO_OPEN_READ 0
#define REB_IO_OPEN_WRITE 1
#define REB_IO_OPEN_APPEND 2

// Open a file. != 0 means failure.
int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle);

// Closes the handle. Same error semantics as open.
int reb_io_close(REB_NUMTYPE handle);

#endif
