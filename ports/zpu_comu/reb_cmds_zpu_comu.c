// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../../reb.h"

extern int digiline_msgc();
extern int digiline_send(char * cptr, int clen, char * dptr, int dlen);
extern int digiline_recv(char * cptr, int clen, char * dptr, int dlen);

REB_CMDS_DEF(reb_cmd_d_send) {
    REB_CMDS_PREFACE;
    // No need to trim here, since it's going to be passed to expression evaluation.
    reb_res_t res = reb_exec_expr(line, env);
    REB_DECREF(line.buf, reb_free_strbuf);
    if (res.errorflag) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        reb_print_val(res, REB_IO_CONSOLE);
        reb_decref_val(res);
        reb_io_puts(REB_IO_CONSOLE, "");
        return result;
    }
    if (res.vtype != REB_RES_CONS) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        reb_decref_val(res);
        reb_io_puts(REB_IO_CONSOLE, "Parameter to d_send must be cons.");
        return result;
    }
    reb_res_t ra = ((reb_cons_t*)res.values.v_cons)->a;
    reb_res_t rb = ((reb_cons_t*)res.values.v_cons)->b;
    // The reference to ra/rb is held by res, which gets decref'd later.
    char * a_buf, * b_buf;
    size_t a_len, b_len;

    if (!reb_pullbuf(ra, &a_buf, &a_len)) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        reb_decref_val(res);
        reb_io_puts(REB_IO_CONSOLE, "d_send channel isn't a string.");
        return result;
    }
    if (!reb_pullbuf(rb, &b_buf, &b_len)) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        reb_decref_val(res);
        reb_io_puts(REB_IO_CONSOLE, "d_send data isn't a string.");
        return result;
    }

    // Truncate too-long messages.
    if (a_len > 255)
        a_len = 255;
    if (b_len > 255)
        b_len = 255;

    // Successfully worked out what the user actually wants sent. Send it.
    digiline_send(a_buf, a_len, b_buf, b_len);

    reb_decref_val(res);
    return result;
}

REB_CMDS_DEF(reb_cmd_d_recv) {
    REB_CMDS_PREFACE;
    // and now, after dealing with possible INPUT#, fix trimming.
    reb_trim(&line);
    reb_strref_t id, id2;
    if (!reb_startsid(&line, &id)) {
        reb_io_puts(REB_IO_CONSOLE, "D_RECV must be given two parameters - variable names.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    reb_trim(&line);
    if (!reb_startsid(&line, &id2)) {
        reb_io_puts(REB_IO_CONSOLE, "D_RECV must be given two parameters - variable names.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    if (line.len != 0) {
        reb_io_puts(REB_IO_CONSOLE, "Only two variable names can be given to D_RECV.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    char * cA, * cB;
    cA = reb_mem_alloc(256);
    if (!cA) {
        reb_io_puts(REB_IO_CONSOLE, "No channel memory");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    cB = reb_mem_alloc(256);
    if (!cB) {
        reb_mem_free(cA);
        reb_io_puts(REB_IO_CONSOLE, "No data memory");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    // Memory allocated.
    while (!digiline_msgc())
        __asm volatile("breakpoint");
    int recvres = digiline_recv(cA, 255, cB, 255);
    int cAL = ((recvres & 0xFF000000) >> 24);
    int cBL = ((recvres & 0xFF0000) >> 16);
    reb_mem_downsize(cA, cAL);
    reb_mem_downsize(cB, cBL);
    reb_strbuf_t * cAB = reb_mem_alloc(sizeof(reb_strbuf_t));
    if (!cAB) {
        reb_mem_free(cA);
        reb_mem_free(cB);
        reb_io_puts(REB_IO_CONSOLE, "No strbuf memory");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    reb_strbuf_t * cBB = reb_mem_alloc(sizeof(reb_strbuf_t));
    if (!cBB) {
        reb_mem_free(cAB);
        reb_mem_free(cA);
        reb_mem_free(cB);
        reb_io_puts(REB_IO_CONSOLE, "No strbuf memory");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    // Finally, all memory allocation is finished.
    cAB->rc = 1; cBB->rc = 1;
    cAB->buf = cA; cBB->buf = cB;
    cAB->len = cAL; cBB->len = cBL;
    REB_INCREF(id.buf);
    REB_INCREF(id2.buf);
    if (reb_execenv_varset(id, reb_strbuf_val(cAB), env)) {
        // Failed.
        reb_mem_free(cBB);
        reb_mem_free(cB);
        reb_io_puts(REB_IO_CONSOLE, "Out Of Memory during varset");
        REB_DECREF(id2.buf, reb_free_strbuf);
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    if (reb_execenv_varset(id2, reb_strbuf_val(cBB), env)) {
        // Again, failed.
        reb_io_puts(REB_IO_CONSOLE, "Out Of Memory during varset");
        result.action = REB_EXECLINEACT_STOP_ERR;        
        // just let the standard decref/return do the rest
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}
