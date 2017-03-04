// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle) {
    char * fnbuf = reb_mem_alloc(namelen + 1);
    if (!fnbuf)
        return -1;
    memcpy(fnbuf, namebuf, namelen);
    fnbuf[namelen] = 0;
    int res = 0;
    if (filemode == REB_IO_OPEN_READ) {
        res = open(fnbuf, O_RDONLY);
    } else if (filemode == REB_IO_OPEN_WRITE) {
        res = creat(fnbuf, 0644);
    } else if (filemode == REB_IO_OPEN_APPEND) {
        res = open(fnbuf, O_CREAT | O_APPEND | O_WRONLY, 0644);
    }
    if (res >= 0) {
     *handle = res;
     return 0;
    }
    return -1;
}

int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * reslen) {
    ssize_t res = read(handle, data, len);
    *reslen = res < 0 ? 0 : res;
    if (res < 0)
        return 1;
    return 0;
}

int reb_io_write(REB_NUMTYPE handle, char * data, size_t len) {
    if (write(handle, data, len) < 0)
        return 1;
    return 0;
}

// reb_io.h makes the real puts illegal
void reb_io_puts(REB_NUMTYPE handle, char * data) {
    reb_io_write(handle, data, strlen(data));
    reb_io_write(handle, "\n", 1);
}

int reb_io_close(REB_NUMTYPE handle) {
    return close(handle);
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

