// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

#include <string.h>

#include <stdio.h>

int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * reslen) {
    for (size_t p = 0; p < len; p++) {
        int r = getchar();
        if (r == -1) {
            *reslen = p;
            return 0;
        }
        data[p] = r;
    }
    *reslen = len;
    return 0;
}

int reb_io_write(REB_NUMTYPE handle, char * data, size_t len) {
    for (size_t p = 0; p < len; p++)
        putchar(data[p]);
    return 0;
}

int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle) {
    return -1;
}

// reb_io.h makes the real puts illegal
void reb_io_puts(REB_NUMTYPE handle, char * data) {
    reb_io_write(handle, data, strlen(data));
    reb_io_write(handle, "\n", 1);
}

int reb_io_close(REB_NUMTYPE handle) {
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
    reb_repl();
    reb_mem_byebye();
    return 0;
}
