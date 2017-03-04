// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

REB_FUNCS_GET_DEF(reb_func_get_sys_fork) {
    reb_decref_val(arg);
    int f = fork();
    if (f == 0)
        env->quit_at_prompt = 1;
    return reb_int(f);
}

REB_FUNCS_GET_DEF(reb_func_get_sys_getpid) {
    reb_decref_val(arg);
    return reb_int(getpid());
}

REB_FUNCS_GET_DEF(reb_func_get_sys_readdir) {
    char * buf;
    size_t len;
    if (reb_pullbuf(arg, &buf, &len)) {
        char * pathname = reb_mem_alloc(len + 1);
        if (!pathname) {
            reb_decref_val(arg);
            return reb_error("sys_readdir: Could not allocate filename buffer.");
        }
        memcpy(pathname, buf, len);
        pathname[len] = 0;
        reb_decref_val(arg);
        // ok, now arg's cleaned up...
        DIR * dir = opendir(pathname);
        reb_mem_free(pathname);
        if (dir == 0)
            return reb_error("sys_readdir: opendir failed.");
        reb_res_t prevval = reb_int(0);
        int has_prevval = 0;
        while (1) {
            struct dirent * ent = readdir(dir);
            if (!ent)
                break;
            reb_res_t res = reb_strbuf_val(reb_string(ent->d_name, strlen(ent->d_name)));
            if (res.errorflag) {
                // Out of memory in the middle of something. *gulp*!
                // Basically, decref previous work out of existence, then break,
                // which should carry the error to the command interpreter.
                reb_decref_val(prevval);
                prevval = res;
                break;
            }
            if (has_prevval) {
                prevval = reb_op_cons(res, prevval);
            } else {
                prevval = res;
            }
            has_prevval = 1;
        }
        closedir(dir);
        return prevval;
    } else {
        reb_decref_val(arg);
        return reb_error("sys_readdir: Filenames are strings");
    }
}
