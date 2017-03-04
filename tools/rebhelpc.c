// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

// Recovery BASIC Help Compiler
// In order to use this, run through a full (cross-)compile from blank.
// Then compile this program for the building machine, and run it.
// STDOUT gives a shell script, which should (after the build is complete)
//  be used to create the HTML version.
#include <stdlib.h>
#include <stdio.h>

// Portable enough.
// At least, more portable than the syscall.
#define MKDIR(dir) system("mkdir " dir);

// Postfix used for links.
#define PF ".md.html"
// Postfix used for building
#define PF_BUILD ".html"

FILE * gen_fileh;
FILE * gen_filei;
char * gen_lpx;

void gen_file(char * fn, char * links) {
    char * buf = strdup(fn);
    char * buf2 = buf;
    while (*buf2) {
        if ((*buf2) == '$')
            (*buf2) = '_';
        buf2++;
    }
    // Definitely not portable, but required to get various things working
    fputs("cat ", stdout);
    fputs(buf, stdout);
    fputs("| sed \"s/</\\&lt\\;/g\" ", stdout);
    fputs("| sed \"s/>/\\&gt\\;/g\" ", stdout);
    fputs("| markdown ", stdout);
    fputs(" > ", stdout);
    fputs(buf, stdout);
    fputs(PF_BUILD, stdout);
    fputs("\n", stdout);
    gen_fileh = fopen(buf, "wb"); // deliberately ensure \r\n
    fputs("[", gen_filei);
    fputs(links, gen_filei);
    fputs("](", gen_filei);
    fputs(buf + 5, gen_filei); // Yes, making a horrifying assumption here
    fputs(PF_BUILD, gen_filei);
    fputs(")\r\n\r\n", gen_filei);
    free(buf);
}
void lpx(char * px) {
    gen_lpx = px;
}
void line(char * l) {
    fputs(gen_lpx, gen_fileh);
    fputs(l, gen_fileh);
    fputs("\r\n", gen_fileh);
}
void gen_endf() {
    fclose(gen_fileh);
}


void gen_cmdhelp() {
    MKDIR("help/cmds")
    fputs("##Commands:\r\n\r\n", gen_filei);
#define REB_CMD_START(caps, ncaps, func) \
    gen_file("help/cmds/" ncaps ".md", caps); lpx(""); line("#" caps); lpx("\r\n");
#define REB_CMD_HELP(str) line(str);
#define REB_CMD_EXAMPLE line("##Example:\r\n"); lpx("    ");
#define REB_CMD_END lpx(""); line(""); gen_endf();
#include "../gen/reb_cmds.h"
}

void gen_funhelp() {
    MKDIR("help/funcs")
    fputs("##Functions:\r\n\r\n", gen_filei);
#define REB_FUNC_START(caps, ncaps, func) \
    gen_file("help/funcs/" ncaps ".md", caps); lpx(""); line("#" caps); lpx("\r\n");
#define REB_FUNC_SET(caps, ncaps, func)  \
    gen_file("help/funcs/" ncaps "_.md", caps " (write)"); lpx(""); line("#" caps); lpx("\r\n");
#define REB_FUNC_HELP(str) line(str);
#define REB_FUNC_EXAMPLE line("##Example:\r\n"); lpx("    ");
#define REB_FUNC_END lpx(""); line(""); gen_endf();
#include "../gen/reb_funcs.h"
}

int main() {
    MKDIR("help");
    gen_filei = fopen("help/index.md", "wb");
    puts("markdown help/index.md > help/index.html");
    gen_cmdhelp();
    gen_funhelp();
    fclose(gen_filei);
}
