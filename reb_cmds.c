// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "reb.h"

REB_CMDS_DEF(reb_cmd_print) {
    REB_CMDS_PREFACE;
    int newline = 1;
    reb_trim(&line);
    REB_NUMTYPE out = REB_IO_CONSOLE;
    if (reb_gethandle_withchecks(&line, &out, env)) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    reb_trim(&line);
    if (line.len != 0) {
        if (line.buf->buf[(line.pos + line.len) - 1] == ';') {
            newline = 1;
            line.len--;
        }
        reb_res_t res = reb_exec_expr(line, env);
        REB_DECREF(line.buf, reb_free_strbuf);
        if (res.errorflag) {
            result.action = REB_EXECLINEACT_STOP_ERR;
            newline = 0;
        }
        reb_print_val(res, out);
        reb_decref_val(res);
    } else {
        REB_DECREF(line.buf, reb_free_strbuf);
    }
    if (newline)
        reb_io_puts(out, "");
    return result;
}

REB_CMDS_DEF(reb_cmd_run) {
    REB_CMDS_PREFACE;
    if (line.len != 0) {
        reb_io_puts(REB_IO_CONSOLE, "RUN does not take any parameters");
        result.action = REB_EXECLINEACT_STOP_ERR;
    } else {
        if (env->firstline) {
            result.action = REB_EXECLINEACT_GOTO;
            result.param = env->firstline->linenum;
        } else {
            reb_io_puts(REB_IO_CONSOLE, "Cannot run - no BASIC program.");
            result.action = REB_EXECLINEACT_STOP_ERR;
        }
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_goto) {
    REB_CMDS_PREFACE;
    if (line.len != 0) {
        reb_res_t res = reb_exec_expr(line, env);
        if (res.errorflag) {
            result.action = REB_EXECLINEACT_STOP_ERR;
            reb_print_val(res, REB_IO_CONSOLE);
            reb_decref_val(res);
            reb_io_puts(REB_IO_CONSOLE, "");
        } else {
            if (res.vtype == REB_RES_NUMBER) {
                result.action = REB_EXECLINEACT_GOTO;
                result.param = res.values.v_number;
            } else {
                reb_io_puts(REB_IO_CONSOLE, "GOTO given something not a number");
                result.action = REB_EXECLINEACT_STOP_ERR;
                reb_decref_val(res);
            }
        }
    } else {
        reb_io_puts(REB_IO_CONSOLE, "GOTO without line number");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

void reb_cmd_save_core(REB_NUMTYPE savehandle, reb_execenv_t * env) {
    reb_exec_lineent_t * ptr = env->firstline;
    while (ptr) {
        reb_res_t lineno = reb_int(ptr->linenum);
        reb_print_val(lineno, savehandle);
        reb_io_write(savehandle, " ", 1);
        reb_print_stringref(ptr->line, savehandle);
        reb_io_puts(savehandle, "");
        ptr = ptr->next;
    }
}

void reb_cmd_clear_core(reb_execenv_t * env) {
    reb_exec_lineent_t * ptr = env->firstline;
    while (ptr) {
        reb_exec_lineent_t * ptrn = ptr->next;
        REB_DECREF(ptr->line.buf, reb_free_strbuf);
        reb_mem_free(ptr);
        ptr = ptrn;
    }
    env->firstline = 0;
}

void reb_cmd_load_core(REB_NUMTYPE savehandle, reb_execenv_t * env) {
    while (1) {
        reb_res_t liner = reb_gets(savehandle);
        if (liner.errorflag) {
            reb_decref_val(liner);
            break;
        }
        if (liner.vtype != REB_RES_STRINGREF) {
            reb_decref_val(liner);
            break;
        }
        reb_strref_t line = liner.values.v_stringref;
        REB_NUMTYPE l = 0;
        // actually a line
        if (reb_linenumber(&line, &l)) {
            reb_trim(&line);
            if (line.len == 0) {
                REB_DECREF(line.buf, reb_free_strbuf);
            } else {
                if (reb_exec_line_env_putline(l, line, env))
                    break;
            }
        } else {
            REB_DECREF(line.buf, reb_free_strbuf);
        }
    }
}

REB_CMDS_DEF(reb_cmd_save) {
    REB_CMDS_PREFACE;
    if (line.len != 0) {
        reb_res_t res = reb_exec_expr(line, env);
        if (res.errorflag) {
            result.action = REB_EXECLINEACT_STOP_ERR;
            reb_print_val(res, REB_IO_CONSOLE);
            reb_decref_val(res);
            reb_io_puts(REB_IO_CONSOLE, "");
        } else {
            char * buf;
            size_t len;
            if (res.vtype == REB_RES_NUMBER) {
                reb_cmd_save_core(res.values.v_number, env);
            } else if (reb_pullbuf(res, &buf, &len)) {
                REB_NUMTYPE fd;
                if (reb_io_open(buf, len, REB_IO_OPEN_WRITE, &fd)) {
                    reb_io_puts(REB_IO_CONSOLE, "Could not open file in SAVE");
                } else {
                    reb_cmd_save_core(fd, env);
                    reb_io_close(fd);
                }
                reb_decref_val(res);
            } else {
                reb_io_puts(REB_IO_CONSOLE, "Bad argument to SAVE");
                result.action = REB_EXECLINEACT_STOP_ERR;
                reb_decref_val(res);
            }
        }
    } else {
        reb_io_puts(REB_IO_CONSOLE, "SAVE without target");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_load) {
    REB_CMDS_PREFACE;
    if (line.len != 0) {
        reb_res_t res = reb_exec_expr(line, env);
        if (res.errorflag) {
            result.action = REB_EXECLINEACT_STOP_ERR;
            reb_print_val(res, REB_IO_CONSOLE);
            reb_decref_val(res);
            reb_io_puts(REB_IO_CONSOLE, "");
        } else {
            char * buf;
            size_t len;
            if (res.vtype == REB_RES_NUMBER) {
                reb_cmd_clear_core(env);
                reb_cmd_load_core(res.values.v_number, env);
            } else if (reb_pullbuf(res, &buf, &len)) {
                REB_NUMTYPE fd;
                if (reb_io_open(buf, len, REB_IO_OPEN_READ, &fd)) {
                    reb_io_puts(REB_IO_CONSOLE, "Could not open file in LOAD");
                } else {
                    reb_cmd_clear_core(env);
                    reb_cmd_load_core(fd, env);
                    reb_io_close(fd);
                }
                reb_decref_val(res);
            } else {
                reb_io_puts(REB_IO_CONSOLE, "Bad argument to LOAD");
                result.action = REB_EXECLINEACT_STOP_ERR;
                reb_decref_val(res);
            }
        }
    } else {
        reb_io_puts(REB_IO_CONSOLE, "LOAD without target");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_list) {
    REB_CMDS_PREFACE;
    if (line.len == 0) {
        reb_cmd_save_core(REB_IO_CONSOLE, env);
    } else {
        reb_io_puts(REB_IO_CONSOLE, "Not a Commodore 64. Use line$().");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_if) {
    REB_CMDS_PREFACE;
    // if _expr_ then - find THEN first
    size_t thenpos;
    if (!reb_search_string(line, "then", "THEN", &thenpos, 1, 1)) {
        reb_io_puts(REB_IO_CONSOLE, "IF without THEN.");
        result.action = REB_EXECLINEACT_STOP_ERR;
    } else {
        reb_strref_t cond, lineno;
        lineno.buf = cond.buf = line.buf;
        // pre and post conditions are used, so the area must exist
        lineno.pos = line.pos + thenpos + 4;
        lineno.len = line.len - (thenpos + 4);

        cond.pos = line.pos;
        cond.len = thenpos;

        reb_res_t condition = reb_exec_expr(cond, env);
        if (condition.errorflag) {
            reb_print_val(condition, REB_IO_CONSOLE);
            reb_decref_val(condition);
            reb_io_puts(REB_IO_CONSOLE, "");
            result.action = REB_EXECLINEACT_STOP_ERR;
            REB_DECREF(line.buf, reb_free_strbuf);
            return result;
        }
        // destroy the actual value - it's useless
        reb_decref_val(condition);
        int fcon = 1;
        if (condition.vtype == REB_RES_NUMBER)
            fcon = (condition.values.v_number != 0);

        // Using reb_cmd_goto consumes reference.
        // Otherwise, the decref afterward does.
        if (fcon)
            return reb_cmd_goto(lineno, env);
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_open) {
    REB_CMDS_PREFACE;
    // open _expr_ for _id_ as _id_
    // Parsing steps:
    // 1. find "for" to divide away first expression.
    // 2. After "for", read 3 IDs, ensure second is "as".
    size_t forpos;
    if (!reb_search_string(line, "for", "FOR", &forpos, 1, 1)) {
        reb_io_puts(REB_IO_CONSOLE, "OPEN without FOR.");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    reb_strref_t filename, opentype, as_id, as_idc, as;
    as_id.buf = filename.buf = line.buf;
    as_id.pos = line.pos + forpos + 3;
    as_id.len = line.len - (forpos + 3);

    filename.pos = line.pos;
    filename.len = forpos;

    reb_trim(&as_id);
    
    if (!reb_startsid(&as_id, &opentype)) {
        reb_io_puts(REB_IO_CONSOLE, "OPEN needs open-type");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    
    reb_trim(&as_id);

    if (!reb_startsid(&as_id, &as)) {
        reb_io_puts(REB_IO_CONSOLE, "OPEN needs 'as'");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }

    reb_trim(&as_id);

    if (!reb_startsid(&as_id, &as_idc)) {
        reb_io_puts(REB_IO_CONSOLE, "OPEN needs target descriptor variable");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    
    if (as_id.len != 0) {
        reb_io_puts(REB_IO_CONSOLE, "Nonsense after OPEN");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    // Final syntax checks (validity of as, open-type)
    int ok = 1;
    if (as.len != 2)
        ok = 0;
    if (ok)
       ok = reb_startswith(&as, "as", "AS", 2);
    if (!ok) {
        reb_io_puts(REB_IO_CONSOLE, "Nonsense in place of \"as\"");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    
    // work out opentype
    int omode = -1;
    ok = 0;
    if (reb_startswith(&opentype, "input", "INPUT", 5)) {
        omode = REB_IO_OPEN_READ;
        ok = opentype.len == 0;
    } else if (reb_startswith(&opentype, "output", "OUTPUT", 6)) {
        omode = REB_IO_OPEN_WRITE;
        ok = opentype.len == 0;
    } else if (reb_startswith(&opentype, "append", "APPEND", 6)) {
        omode = REB_IO_OPEN_APPEND;
        ok = opentype.len == 0;
    }
    if (!ok) {
        reb_io_puts(REB_IO_CONSOLE, "open-type not valid (input/output/append)");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
    } else {
        // Now for actual command execution...
        reb_res_t file = reb_exec_expr(filename, env);
        char * namebuf;
        size_t namelen;
        if (!reb_pullbuf(file, &namebuf, &namelen)) {
            reb_decref_val(file);
            reb_io_puts(REB_IO_CONSOLE, "Filename wasn't a string.");
            result.action = REB_EXECLINEACT_STOP_ERR;
            REB_DECREF(line.buf, reb_free_strbuf);
            return result;
        }
        int handle = 0;
        if (reb_io_open(namebuf, namelen, omode, &handle)) {
            // oh well
            reb_decref_val(file);
            reb_io_puts(REB_IO_CONSOLE, "File open failed.");
            result.action = REB_EXECLINEACT_STOP_ERR;
            REB_DECREF(line.buf, reb_free_strbuf);
            return result;
        }
        reb_decref_val(file);
        // done, now to remove the reference in a varset.
        if (reb_execenv_varset(as_idc, reb_int(handle), env)) {
            // well THAT's a mess
            reb_io_close(handle);
            reb_io_puts(REB_IO_CONSOLE, "Varset failed");
            result.action = REB_EXECLINEACT_STOP_ERR;
        }
    }
    return result;
}

REB_CMDS_DEF(reb_cmd_input) {
    REB_CMDS_PREFACE;
    reb_trim(&line);
    REB_NUMTYPE kb = REB_IO_KEYBOARD;
    if (reb_gethandle_withchecks(&line, &kb, env)) {
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
        return result;
    }
    // and now, after dealing with possible INPUT#, fix trimming.
    reb_trim(&line);
    reb_strref_t id;
    if (!reb_startsid(&line, &id)) {
        reb_io_puts(REB_IO_CONSOLE, "INPUT must be given one, and only one parameter: variable name.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    if (line.len != 0) {
        reb_io_puts(REB_IO_CONSOLE, "Only one variable name can be given to INPUT.");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    int numeric = id.buf->buf[id.pos + id.len - 1] != '$';
    reb_res_t res = reb_gets(kb);
    if (numeric) {
        REB_NUMTYPE val = 0;
        
        while (!res.errorflag) {
            if (res.vtype == REB_RES_NUMBER)
                break;
            reb_strref_t inl = res.values.v_stringref;
            reb_trim(&inl);
            if (inl.len == 0)
                break;
            if (reb_linenumber(&inl, &val))
                if (inl.len == 0)
                    break;
            
            reb_decref_val(res);
            res = reb_gets(kb);
        }
        if (!res.errorflag) {
            if (res.vtype == REB_RES_NUMBER) {
                res = reb_op_cons(reb_int(0), reb_int(0));
            } else {
                reb_decref_val(res);
                res.errorflag = 0;
                res.vtype = REB_RES_NUMBER;
                res.values.v_number = val;
            }
        }
    } else {
        if (!res.errorflag)
            if (res.vtype == REB_RES_NUMBER)
                res = reb_op_cons(reb_int(0), reb_int(0));
    }
    if (res.errorflag) {
        reb_print_val(res, REB_IO_CONSOLE);
        reb_decref_val(res);
        reb_io_puts(REB_IO_CONSOLE, "");
        REB_DECREF(line.buf, reb_free_strbuf);
        result.action = REB_EXECLINEACT_STOP_ERR;
        return result;
    }
    // this acts as the line consumer
    if (reb_execenv_varset(id, res, env)) {
        // so apparently we're out of memory
        reb_io_puts(REB_IO_CONSOLE, "Out Of Memory");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    return result;
}

REB_CMDS_DEF(reb_cmd_end) {
    REB_CMDS_PREFACE;
    if (line.len == 0) {
        result.action = REB_EXECLINEACT_STOP;
    } else {
        reb_io_puts(REB_IO_CONSOLE, "END does not take any parameters.");
        result.action = REB_EXECLINEACT_STOP_ERR;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

REB_CMDS_DEF(reb_cmd_close) {
    REB_CMDS_PREFACE;
    reb_trim(&line);
    if (line.len != 0) {
        reb_res_t res = reb_exec_expr(line, env);
        REB_DECREF(line.buf, reb_free_strbuf);
        if (res.errorflag) {
            reb_print_val(res, REB_IO_CONSOLE);
            result.action = REB_EXECLINEACT_STOP_ERR;
            return result;
        }
        if (res.vtype != REB_RES_NUMBER) {
            reb_decref_val(res);
            reb_io_puts(REB_IO_CONSOLE, "CLOSE requires a handle.");
            result.action = REB_EXECLINEACT_STOP_ERR;
            return result;
        }
        if (reb_io_close(res.values.v_number)) {
            reb_io_puts(REB_IO_CONSOLE, "CLOSE failed.");
            result.action = REB_EXECLINEACT_STOP_ERR;
        }
    } else {
        reb_io_puts(REB_IO_CONSOLE, "CLOSE what?");
        result.action = REB_EXECLINEACT_STOP_ERR;
        REB_DECREF(line.buf, reb_free_strbuf);
    }
    return result;
}

// THE REM RANT: CONTINUATION
// In fact, this command is so useless I've spent more lines ranting about it than the actual command itself takes up.
// In C. Even if you expanded all the macros and inlined the one function call.
REB_CMDS_DEF(reb_cmd_rem) {
    REB_CMDS_PREFACE;
    REB_DECREF(line.buf, reb_free_strbuf);
    return result;
}

#undef REB_CMDS_FUNC
#undef REB_CMDS_PREFACE
