// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

// Assembly routines that are, theoretically, fast.
extern size_t strlen(const char * str);
extern void * memchr(const void * p, int i, size_t l);
extern void _memcpy_fast4(char * s1, char * s2, size_t n);

// In any case, it's nice to see the logic behind them neatly abstracted away.
extern int digiline_msgc();
extern int digiline_send(char * cptr, int clen, char * dptr, int dlen);
// Note: returns the Control Word
extern int digiline_recv(char * cptr, int clen, char * dptr, int dlen);

// things defined here
int memcmp(const void * s1, const void * s2, size_t n);
void * memcpy(void * s1, void * s2, size_t n);

static char conbuf[255];
static int conbuf_av;
static char * conbuf_ptr;

// Implements REB_IO_NEWLINE_ENDS_READ by relying on the ~quirks~ FEATURES
//  in the Digilines Console. This is required for speed.
int reb_io_read(REB_NUMTYPE handle, char * data, size_t len, size_t * reslen) {
    while (!conbuf_av) {
        char bufA[9];
        if (len >= 255) {
            // Alternate hyper-optimized read for these cases
            while (1) {
                while (!digiline_msgc())
                    __asm("breakpoint");
                int res = digiline_recv(bufA, 9, data, 255);
                if ((res & 0xFF000000) == 0x8000000) {
                    if (!memcmp(bufA, "keyboard", 8)) {
                        // Message received on console, add to buffer.
                        *reslen = ((res & 0xFF0000) >> 16);
                        return 0;
                    }
                }
            }
        }
        // Wait for incoming messages.
        while (!digiline_msgc())
            __asm volatile("breakpoint");
        // Read them...
        int res = digiline_recv(bufA, 9, conbuf, 255);
        if ((res & 0xFF000000) == 0x8000000) {
            if (!memcmp(bufA, "keyboard", 8)) {
                // Message received on console, add to buffer.
                conbuf_ptr = conbuf;
                conbuf_av = (res & 0xFF0000) >> 16;
            }
        }
    }

    if (conbuf_av < len)
        len = conbuf_av;
    memcpy(data, conbuf_ptr, len);
    conbuf_av -= len;
    conbuf_ptr += len;
    *reslen = len;
    return 0;
}

int reb_io_write(REB_NUMTYPE handle, char * data, size_t len) {
    while (len > 255) {
        digiline_send("console", 7, data, 255);
        data += 255;
        len -= 255;
    }
    digiline_send("console", 7, data, len);
    return 0;
}

int reb_io_open(char * namebuf, size_t namelen, int filemode, REB_NUMTYPE * handle) {
    return -1;
}

// reb_io.h makes the real puts illegal
void reb_io_puts(REB_NUMTYPE handle, char * data) {
    char nl = '\n';
    reb_io_write(handle, data, strlen(data));
    digiline_send("console", 7, &nl, 1);
}

int reb_io_close(REB_NUMTYPE handle) {
    return 0;
}

#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
extern char _end;
extern size_t _memory_end;
void main() {
    conbuf_av = 0;
    reb_mem_start(&_end, (_memory_end - ((intptr_t) (&_end))) - 0x2000);
#else
void main() {
    reb_mem_start();
#endif
    reb_repl();
    reb_mem_byebye();
}

// Implementations for stuff

// definition doesn't match, this is fine.
int memcmp(const void * s1, const void * s2, size_t n) {
    while ((n--) > 0)
        if (*((char*)(s1++)) != *((char*)(s2++)))
            return 1;
    return 0;
}

void * memcpy(void * s1, void * s2, size_t n) {
    void * os1 = s1;
    if (!((((intptr_t)s1) & 3) || (((intptr_t)s2) & 3))) {
        size_t np = n & 0xFFFFFFFC;
        if (np)
            _memcpy_fast4(s1, s2, np);
        s1 += np;
        s2 += np;
        n &= 3;
    }
    while ((n--) > 0)
        *((char*)(s1++)) = *((char*)(s2++));
    return os1;
}

void __assert(char * expr, char * file, int line) {
    reb_io_puts(0, "Assertion failure. How'd you manage that?");
    reb_io_puts(0, expr);
    reb_io_puts(0, file);
    while (1)
        __asm("breakpoint");
}
