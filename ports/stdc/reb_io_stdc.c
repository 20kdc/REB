// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

#include <string.h>

#include <stdio.h>

// handle manager

#define REB_IO_HANDLES 32

static FILE * reb_io_handles[REB_IO_HANDLES];

static int reb_io_valid(REB_NUMTYPE handle) {
    if (handle < 0)
        return 0;
    if (handle >= REB_IO_HANDLES)
        return 0;
    if (reb_io_handles[handle] == NULL)
        return 0;
    return 1;
}

// implementation

int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * reslen) {
    FILE * fn;
    if (!reb_io_valid(handle))
        return -1;
    fn = reb_io_handles[handle];
    for (size_t p = 0; p < len; p++) {
        int r = fgetc(fn);
        if (r == EOF) {
            *reslen = p;
            return 0;
        }
        data[p] = r;
    }
    *reslen = len;
    return 0;
}

int reb_io_write(REB_NUMTYPE handle, char * data, size_t len) {
    FILE * fn;
    if (!reb_io_valid(handle))
        return -1;
    fn = reb_io_handles[handle];
    for (size_t p = 0; p < len; p++)
        putc(data[p], fn);
    return 0;
}

int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle) {
    // Find empty file handle slot
    for (int i = 0; i < REB_IO_HANDLES; i++) {
        if (!reb_io_handles[i]) {
            // Empty file handle slot
            const char * mode;
            if (filemode == REB_IO_OPEN_READ) {
                mode = "rb";
            } else if (filemode == REB_IO_OPEN_WRITE) {
                mode = "wb";
            } else if (filemode == REB_IO_OPEN_APPEND) {
                mode = "ab";
            } else {
                return -1;
            }
            char * tmpname = reb_mem_alloc(namelen + 1);
            if (!tmpname)
                return -1;
            memcpy(tmpname, namebuf, namelen);
            tmpname[namelen] = 0;
            reb_io_handles[i] = fopen(tmpname, mode);
            reb_mem_free(tmpname);
            if (reb_io_handles[i]) {
                *handle = i;
                return 0;
            } else {
                return -1;
            }
        }
    }
    return -1;
}

// reb_io.h makes the real puts illegal
void reb_io_puts(REB_NUMTYPE handle, char * data) {
    reb_io_write(handle, data, strlen(data));
    reb_io_write(handle, "\n", 1);
}

int reb_io_close(REB_NUMTYPE handle) {
    if (handle < 2)
        return -1;
    if (!reb_io_valid(handle))
        return -1;
    fclose(reb_io_handles[handle]);
    reb_io_handles[handle] = NULL;
    return 0;
}
#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
static char reb_memory[REB_MEM_CONFIG_CUSTOMALLOCATOR];

int main(int argc, char ** argv) {
    reb_mem_start(reb_memory, REB_MEM_CONFIG_CUSTOMALLOCATOR);
#else
int main(int argc, char ** argv) {
    reb_mem_start();
#endif
    reb_io_handles[0] = stdout;
    reb_io_handles[1] = stdin;
    reb_repl();
    reb_mem_byebye();
    return 0;
}
