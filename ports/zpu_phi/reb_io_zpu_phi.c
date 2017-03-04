// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

extern int inbyte();
extern void outbyte(int c);

int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * reslen) {
    for (size_t p = 0; p < len; p++)
        data[p] = inbyte();
    *reslen = len;
    return 0;
}

int reb_io_write(REB_NUMTYPE handle, char * data, size_t len) {
    for (size_t p = 0; p < len; p++)
        outbyte(data[p]);
    return 0;
}

int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle) {
    return -1;
}

// reb_io.h makes the real puts illegal
void reb_io_puts(REB_NUMTYPE handle, char * data) {
    while (*data)
        outbyte(*(data++));
    outbyte(10);
}

int reb_io_close(REB_NUMTYPE handle) {
    return 0;
}

#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
static char reb_memory[REB_MEM_CONFIG_CUSTOMALLOCATOR];

void main() {
    reb_mem_start(reb_memory, REB_MEM_CONFIG_CUSTOMALLOCATOR);
#else
void main() {
    reb_mem_start();
#endif
    reb_repl();
    reb_mem_byebye();
}

// Implementations for stuff

// definition doesn't match, this is fine.
int memcmp(const char * s1, const char * s2, size_t n) {
    for (size_t p = 0; p < n; p++) {
        if (*s1 != *s2)
            return 1;
        s1++;
        s2++;
    }
    return 0;
}

void memcpy(char * s1, char * s2, size_t n) {
    for (size_t p = 0; p < n; p++)
        *(s1++) = *(s2++);
}

size_t strlen(char * s) {
    size_t a = 0;
    while (*(s++))
        a++;
    return a;
}

void * memchr(void * p, int i, size_t l) {
    while (l--) {
        if (*((unsigned char*)p) == i)
            return p;
        p++;
    }
    return 0;
}

void __assert(char * expr, char * file, int line) {
    reb_io_puts(0, "Assertion failure. How'd you manage that?");
    reb_io_puts(0, expr);
    reb_io_puts(0, file);
    while (1) {
        inbyte();
    }
}
