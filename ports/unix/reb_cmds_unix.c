// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include <unistd.h>
#include "../../reb.h"

REB_CMDS_DEF(reb_cmd_sys_sync) {
    REB_CMDS_PREFACE;
    if (line.len == 0) {
        sync();
    } else {
        reb_io_puts(REB_IO_CONSOLE, "SYS_SYNC does not take any parameters.");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_sys_pipe) {
    REB_CMDS_PREFACE;
    // and now, after dealing with possible INPUT#, fix trimming.
    reb_trim(&line);
    reb_strref_t id, id2;
    if (!reb_startsid(&line, &id)) {
        reb_io_puts(REB_IO_CONSOLE, "SYS_PIPE must be given two parameters - variable names.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    reb_trim(&line);
    if (!reb_startsid(&line, &id2)) {
        reb_io_puts(REB_IO_CONSOLE, "SYS_PIPE must be given two parameters - variable names.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    if (line.len != 0) {
        reb_io_puts(REB_IO_CONSOLE, "Only two variable names can be given to SYS_PIPE.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    int pipefds[2];
    // let's be honest: if this fails, you're doomed
    if (pipe(pipefds)) {
        reb_io_puts(REB_IO_CONSOLE, "SYS_PIPE failed.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    REB_INCREF(id.buf);
    REB_INCREF(id2.buf);
    if (reb_execenv_varset(id, reb_int(pipefds[0]), env)) {
        // Failed.
        reb_io_puts(REB_IO_CONSOLE, "Out Of Memory during varset; pipe[0]");
        close(pipefds[0]);
        close(pipefds[1]);
        REB_DECREF(id2.buf, reb_free_strbuf);
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    if (reb_execenv_varset(id2, reb_int(pipefds[1]), env)) {
        // Failed after one had been written - yikes. Nevertheless...
        reb_io_puts(REB_IO_CONSOLE, "Out Of Memory during varset; pipe[1]");
        close(pipefds[0]);
        close(pipefds[1]);
        result.action = REB_EXECLINEACT_STOP_ERR;        
        // just let the standard decref/return do the rest
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}
