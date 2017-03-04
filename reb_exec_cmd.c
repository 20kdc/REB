// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "reb.h"

int reb_startswith(reb_strref_t * haystack, char * needle, char * needlecaps, size_t needlelen) {
    if (haystack->len < needlelen)
        return 0;
    char * hs = haystack->buf->buf + haystack->pos;
    if (memcmp(hs, needle, needlelen))
        if (memcmp(hs, needlecaps, needlelen))
            return 0;
    haystack->pos += needlelen;
    haystack->len -= needlelen;
    return 1;
}

int reb_linenumber(reb_strref_t * haystack, REB_NUMTYPE * ret) {
    int sawnum = 0;
    int negative = 0;
    *ret = 0;
    while (haystack->len > 0) {
        char c = haystack->buf->buf[haystack->pos];
        if (!sawnum) {
            if (!negative) {
                if (c == '-') {
                    negative = 1;
                    haystack->pos++;
                    haystack->len--;
                    continue;
                }
            }
        }
        if (!((c >= '0') && (c <= '9')))
            break;
        *ret *= 10;
        *ret += c - '0';
        sawnum = 1;
        haystack->pos++;
        haystack->len--;
    }
    if (negative)
        *ret = -(*ret);
    return sawnum;
}

int reb_gethandle_withchecks(reb_strref_t * line, REB_NUMTYPE * ret, reb_execenv_t * env) {
    reb_res_t kb = reb_gethandle(line, *ret, env);
    if (kb.errorflag) {
        reb_print_val(kb, REB_IO_CONSOLE);
        reb_decref_val(kb);
        reb_io_puts(REB_IO_CONSOLE, "");
        REB_DECREF(line->buf, reb_free_strbuf);
        return 1;
    }
    if (kb.vtype != REB_RES_NUMBER) {
        reb_io_puts(REB_IO_CONSOLE, "IO-with-handle requires the handle is valid.");
        REB_DECREF(line->buf, reb_free_strbuf);
        return 1;
    }
    *ret = kb.values.v_number;
    return 0;
}

static void reb_readtoeol(REB_NUMTYPE f) {
    char ch;
    size_t res_sz;
    while (1) {
        int res_err = reb_io_read(f, &ch, 1, &res_sz);
        if (res_err)
            break;
        if (res_sz != 1)
            break;
        if (ch == 10)
            break;
    }
}

// Returns reb_num(0) on IO error, reb_error(...) for other kinds
// Uses an initial buffer for the first few characters,
//  then transfers it if need be.
// speedbuf_al is a boolean which specifies if the speedup buffer is on-heap
//  (the reference is transferred to the function if so)

// If the speedbuf is on-heap, then "emergency" MUST be 0.
// Otherwise, it is a pointer to an on-stack strbuf holding speedbuf,
//  with a refcount of 1 or more.
// If the returned reb_res_t's errorflag is off,
//  and res.values.v_stringref.buf == emergency,
//  then *emergency handling is in use*.
// Return value reference given to caller.
// "emergency" reference not taken (it creates a new one if needed)

reb_res_t reb_gets_internal(REB_NUMTYPE f, char * speedbuf, int speedbuf_al, size_t speedbuf_sz, reb_strbuf_t * emergency) {
    size_t speedbuf_alp = 0;
    char * speedbuf_ptr = speedbuf;
    while (1) {
        for (size_t i = speedbuf_alp; i < speedbuf_sz; i++) {
            size_t res_sz;
#ifdef REB_IO_NEWLINE_ENDS_READ
            int res_err = reb_io_read(f, speedbuf_ptr, speedbuf_sz - i, &res_sz);
#else
            int res_err = reb_io_read(f, speedbuf_ptr, 1, &res_sz);
#endif
            if ((!res_err) && (res_sz > 0)) {
#ifdef REB_IO_NEWLINE_ENDS_READ
                i += res_sz - 1; // remaining 1 dealt with later
#endif
                speedbuf_ptr += res_sz;
                if (speedbuf[i] == 10) {
                    reb_strbuf_t * sb = reb_mem_alloc(sizeof(reb_strbuf_t));
                    if (sb) {
                        sb->rc = 1;
                        sb->len = i;
                        if (speedbuf_al) {
                            // speedbuf is on heap, not on stack.
                            sb->buf = speedbuf;
                            reb_mem_downsize(speedbuf, i);
                        } else {
                            if (i) {
                                sb->buf = reb_mem_alloc(i);
                                if (!sb->buf) {
                                    reb_mem_free(sb);
                                    sb = 0;
                                } else {
                                    memcpy(sb->buf, speedbuf, i);
                                }
                            } else {
                                sb->buf = 0;
                            }
                        }
                    } else {
                        if (speedbuf_al)
                            reb_mem_free(speedbuf);
                    }
                    reb_res_t strb = reb_strbuf_val(sb);
                    if (strb.errorflag) {
                        // Try emergency handling.
                        if (emergency) {
                            REB_INCREF(emergency);
                            reb_res_t e = reb_strbuf_val(emergency);
                            e.values.v_stringref.len = i;
                            return e;
                        }
                    }
                    return strb;
                }
            } else if (res_err == 0) {
                if (speedbuf_al)
                    reb_mem_free(speedbuf);
                return reb_int(0);
            } else {
                if (speedbuf_al)
                    reb_mem_free(speedbuf);
                return reb_error("IO Error");
            }
        }
        // Ran out of buffer, allocate more.
        char * nx = reb_mem_alloc(speedbuf_sz + 4);
        if (!nx) {
            if (speedbuf_al)
                reb_mem_free(speedbuf);
            reb_readtoeol(f);
            return reb_error("reb_gets OOM");
        }

        memcpy(nx, speedbuf, speedbuf_sz);
        if (speedbuf_al)
            reb_mem_free(speedbuf);
        speedbuf = nx;

        speedbuf_al = 1;
        speedbuf_alp = speedbuf_sz;
        speedbuf_ptr = nx + speedbuf_sz;
        speedbuf_sz += 4;
        // No longer using original memory, so switch off emergency handling
        emergency = 0;
    }
}

// Wraps reb_gets_internal with a default buffer and no emergency handling.
reb_res_t reb_gets(REB_NUMTYPE f) {
    char speedbuf[32];
    return reb_gets_internal(f, speedbuf, 0, 32, 0);
}
lookuptree_t * reb_exec_line_env_getvar(reb_strref_t line, reb_execenv_t * env, int expand) {
    lookuptree_t * ptr = &(env->variables);
    for (size_t i = 0; i < line.len; i++) {
        char chr = line.buf->buf[i + line.pos];
        if ((chr >= 'a') && (chr <= 'z')) {
            chr -= 'a';
            chr += 'A';
        }
        ptr = lookuptree_navigate(ptr, chr, expand);
        if (ptr == 0)
            break;
    }
    REB_DECREF(line.buf, reb_free_strbuf);
    return ptr;
}

reb_exec_lineent_t * reb_exec_line_env_getline(REB_NUMTYPE line, reb_execenv_t * env) {
    reb_exec_lineent_t * ptr = env->firstline;
    while (ptr) {
        if (ptr->linenum > line)
            return 0;
        if (ptr->linenum == line)
            return ptr;
        ptr = ptr->next;
    }
    return ptr;
}

void reb_exec_line_env_delline(REB_NUMTYPE line, reb_execenv_t * env) {
    reb_exec_lineent_t * ptr = env->firstline;
    reb_exec_lineent_t ** writepoint = &(env->firstline);
    while (ptr) {
        if (ptr->linenum > line)
            return; // the line didn't exist anyway
        if (ptr->linenum == line) {
            *writepoint = ptr->next; // Erase it from the linked list
            REB_DECREF(ptr->line.buf, reb_free_strbuf); // and kill the ref.
            reb_mem_free(ptr);
        }
        // In other news, GCC developers today gave a news report:
        // "We are striving to make the compiler as obsessive as possible,
        //  over the very slightest details of the code.
        //  Avoiding warnings from GCC will be as hard as writing C++.
        //  Maybe a bit more."
        // Cause of the above little joke:
        //  writepoint = &(ptr->next);
        // ^ apparently void* is no longer a convertible pointer type *WARNING*
        //  writepoint = &((reb_exec_lineent_t*)ptr->next);
        // ^ invalid!
        //  writepoint = (reb_exec_lineent_t**) (&(ptr->next));
        // ^ GCC-Approved (TM)...

        writepoint = (reb_exec_lineent_t**) (&(ptr->next));
        ptr = ptr->next;
    }
    // got to end without even reaching evidence the line doesn't exist
}

// Transfers ownership to the newly added line.

int reb_exec_line_env_putline(REB_NUMTYPE line, reb_strref_t lineref, reb_execenv_t * env) {
    reb_exec_lineent_t * ptr = env->firstline;
    reb_exec_lineent_t ** writepoint = &(env->firstline);
    while (ptr) {
        if (ptr->linenum > line)
            break;
        if (ptr->linenum == line) {
            // overwrite
            reb_strref_t old = ptr->line;
            ptr->line = lineref;
            REB_DECREF(old.buf, reb_free_strbuf);
            return 0;
        }
        writepoint = (reb_exec_lineent_t**) (&(ptr->next));
        ptr = ptr->next;
    }
    // first, ptr points at the next line.
    // second, writepoint is the "next" pointer of the previous line.
    reb_exec_lineent_t * alloc = reb_mem_alloc(sizeof (reb_exec_lineent_t));
    if (!alloc) {
        REB_DECREF(lineref.buf, reb_free_strbuf);
        return 1;
    }
    *writepoint = alloc;
    alloc->line = lineref;
    alloc->linenum = line;
    alloc->next = ptr;
    return 0;
}

reb_exec_line_act_t reb_exec_line(reb_strref_t line, reb_execenv_t * env) {
    reb_exec_line_act_t result;
    reb_trim(&line);
    REB_NUMTYPE l = 0;
    result.action = REB_EXECLINEACT_CONTINUE;
    result.param = 0;
    if (reb_linenumber(&line, &l)) {
        reb_trim(&line);
        if (line.len == 0) {
            REB_DECREF(line.buf, reb_free_strbuf);
            reb_exec_line_env_delline(l, env);
        } else {
            if (reb_exec_line_env_putline(l, line, env)) {
                reb_io_puts(REB_IO_CONSOLE, "Out of memory during putline");
                result.action = REB_EXECLINEACT_STOP_ERR;
                return result;
            }
        }
    } else {

        // This line shouldn't exist, but it can be inputted via the console
        if (line.len == 0) {
            REB_DECREF(line.buf, reb_free_strbuf);
            return result;
        }

        reb_strref_t id;
        if (reb_startswith(&line, "help", "HELP", 4)) {
            reb_trim(&line);
            if (line.len == 0) {
#define REB_CMD_START(caps, nocaps, func) " " caps
#define REB_CMD_HELP(str)
#define REB_CMD_EXAMPLE
#define REB_CMD_END
                reb_io_puts(REB_IO_CONSOLE, "available commands:"
#include "gen/reb_cmds.h"
                        );
#undef REB_CMD_START
#undef REB_CMD_HELP
#undef REB_CMD_EXAMPLE
#undef REB_CMD_END
#define REB_FUNC_START(caps, nocaps, func) " " caps "("
#define REB_FUNC_SET(caps, nocaps, func)
#define REB_FUNC_HELP(str)
#define REB_FUNC_EXAMPLE
#define REB_FUNC_END
                reb_io_puts(REB_IO_CONSOLE, "available functions:"
#include "gen/reb_funcs.h"
                        );

#undef REB_FUNC_START
#undef REB_FUNC_SET
#define REB_FUNC_START(caps, nocaps, func)
#define REB_FUNC_SET(caps, nocaps, func) " " caps "(="

                reb_io_puts(REB_IO_CONSOLE, "available writing functions:"
#include "gen/reb_funcs.h"
                        );

#undef REB_FUNC_START
#undef REB_FUNC_SET
#undef REB_FUNC_HELP
#undef REB_FUNC_EXAMPLE
#undef REB_FUNC_END
            } else {
#ifndef REB_NO_HELP
                int ok = 0;
                int example = 0;
#define REB_CMD_START(caps, nocaps, func) if (line.len == (sizeof(caps) - 1)) { if (reb_startswith(&line, nocaps, caps, sizeof(caps) - 1)) { ok = 1;
#define REB_CMD_HELP(str) if (example) {reb_io_puts(REB_IO_CONSOLE, " " str);} else {reb_io_puts(REB_IO_CONSOLE, str);}
#define REB_CMD_EXAMPLE REB_CMD_HELP("Example:") example = 1;
#define REB_CMD_END } }

#define REB_FUNC_START(caps, nocaps, func) REB_CMD_START(caps "(", nocaps "(", func)
#define REB_FUNC_SET(caps, nocaps, func) REB_CMD_START(caps "(=", nocaps "(", func)
#define REB_FUNC_HELP(str) REB_CMD_HELP(str)
#define REB_FUNC_EXAMPLE REB_CMD_EXAMPLE
#define REB_FUNC_END } }

#include "gen/reb_cmds.h"
#include "gen/reb_funcs.h"

#undef REB_CMD_START
#undef REB_CMD_HELP
#undef REB_CMD_EXAMPLE
#undef REB_CMD_END
#undef REB_FUNC_START
#undef REB_FUNC_SET
#undef REB_FUNC_HELP
#undef REB_FUNC_EXAMPLE
#undef REB_FUNC_END
                if (!ok)
                    reb_io_puts(REB_IO_CONSOLE, "No such command or function!");
#else
                reb_io_puts(REB_IO_CONSOLE, "Helpfiles not compiled");
#endif
            }
            REB_DECREF(line.buf, reb_free_strbuf);
#define REB_CMD_START(caps, nocaps, func) } else if (reb_startswith(&line, nocaps, caps, sizeof(caps) - 1)) { result = func(line, env);
#define REB_CMD_HELP(str)
#define REB_CMD_EXAMPLE
#define REB_CMD_END
#include "gen/reb_cmds.h"
#undef REB_CMD_START
#undef REB_CMD_HELP
#undef REB_CMD_EXAMPLE
#undef REB_CMD_END
        } else if (reb_startsid(&line, &id)) {
            // ignore id as far as refcounting goes.
            // If there's *only* the ID, then other will be 0 because of the trimming.
            if (line.len == 0) {
                reb_io_puts(REB_IO_CONSOLE, "Expected (<lineNum> EOL | (<lineNum> SPC+)? <command> EOL | (<lineNum> SPC+)? <variable> = <expr> EOL).");
                result.action = REB_EXECLINEACT_STOP_ERR;
                REB_DECREF(line.buf, reb_free_strbuf);
            } else {
                // =?
                reb_trim(&line);
                size_t eq_pos;
                if (reb_search_string(line, "=", "=", &eq_pos, 0, 0)) {
                    // start to eq_pos - 1: iExpr continuation (array index?)
                    // eq_pos + 1 to end: Expr
                    // This opens up the possibility of: line$(5) = "print ";q;"dynamic modification";q
                    // that's mostly a niche feature, but the point should be clear
                    if (eq_pos != 0) {
                        reb_strref_t index;
                        index.buf = line.buf;
                        index.pos = line.pos;
                        index.len = eq_pos;
                        reb_res_t idx = reb_exec_expr(index, env);
                        if (idx.errorflag) {
                            result.action = REB_EXECLINEACT_STOP_ERR;
                            reb_print_val(idx, REB_IO_CONSOLE);
                            reb_decref_val(idx);
                            reb_io_puts(REB_IO_CONSOLE, "");
                            // and the reference held for line/id/etc is now useless
                            REB_DECREF(id.buf, reb_free_strbuf);
                        } else {
                            line.pos += eq_pos + 1;
                            line.len -= eq_pos + 1;

                            reb_res_t arg = reb_exec_expr(line, env);
                            if (arg.errorflag) {
                                result.action = REB_EXECLINEACT_STOP_ERR;
                                reb_print_val(arg, REB_IO_CONSOLE);
                                reb_io_puts(REB_IO_CONSOLE, "");
                                reb_decref_val(arg);
                                reb_decref_val(idx);
                                // and the reference held for line/id/etc is now useless
                                REB_DECREF(id.buf, reb_free_strbuf);
                            } else {
                                // now just to set it (consumes references)
                                reb_res_t res = reb_execenv_sfunccall(id, idx, arg, env);
                                if (res.errorflag) {
                                    result.action = REB_EXECLINEACT_STOP_ERR;
                                    reb_print_val(res, REB_IO_CONSOLE);
                                    reb_io_puts(REB_IO_CONSOLE, "");
                                }
                                reb_decref_val(res);
                            }
                        }
                    } else {
                        // eq_pos == 0, so just skip over that,
                        line.pos++;
                        line.len--;
                        // Reminder:
                        reb_res_t res = reb_exec_expr(line, env);
                        if (res.errorflag) {
                            result.action = REB_EXECLINEACT_STOP_ERR;
                            reb_print_val(res, REB_IO_CONSOLE);
                            reb_decref_val(res);
                            reb_io_puts(REB_IO_CONSOLE, "");
                            // and the reference held for line/id/etc is now useless
                            REB_DECREF(id.buf, reb_free_strbuf);
                        } else {
                            // now just to set it (consumes line/id/etc reference)
                            if (reb_execenv_varset(id, res, env)) {
                                reb_io_puts(REB_IO_CONSOLE, "Out Of Memory");
                                result.action = REB_EXECLINEACT_STOP_ERR;
                            }
                        }
                    }
                } else {
                    reb_io_puts(REB_IO_CONSOLE, "Expected (command EOL | variable = <expr> EOL).");
                    result.action = REB_EXECLINEACT_STOP_ERR;
                    REB_DECREF(line.buf, reb_free_strbuf);
                }
            }
        } else {
            reb_io_puts(REB_IO_CONSOLE, "Unknown command");
            result.action = REB_EXECLINEACT_STOP_ERR;
            REB_DECREF(line.buf, reb_free_strbuf);
        }
    }
    lookuptree_prune(&(env->variables), 0);
    return result;
}

reb_res_t reb_execenv_varget(reb_strref_t id, reb_execenv_t * ud) {
    // eats reference to id
    lookuptree_t * ent = reb_exec_line_env_getvar(id, ud, 0);
    if (ent) {
        if (ent->entry) {
            reb_res_t val = *((reb_res_t*) (ent->entry));
            reb_incref_val(val);
            return val;
        }
    }
    // The variable didn't exist, use a default value.
    return reb_int(0);
}

int reb_execenv_varset(reb_strref_t id, reb_res_t val, reb_execenv_t * ud) {
    lookuptree_t * ent = reb_exec_line_env_getvar(id, ud, 1);
    if (!ent) {
        reb_decref_val(val);
        return 1;
    }
    int defaultval = 0;
    if (val.vtype == REB_RES_NUMBER)
        if (val.values.v_number == 0)
            defaultval = 1;
    // default-val is so that variables can be deallocated
    // useful in low-memory environments or when checking for memory leaks
    if (ent->entry) {
        reb_res_t oldval = *((reb_res_t*) (ent->entry));
        reb_decref_val(oldval);
        if (defaultval) {
            reb_mem_free(ent->entry);
            ent->entry = 0;
            // another "val is number so no allocation madness" case
            return 0;
        }
    } else {
        if (defaultval)
            return 0;
        ent->entry = reb_mem_alloc(sizeof (reb_res_t));
        if (!ent->entry) {
            reb_decref_val(val);
            return 1;
        }
    }
    *((reb_res_t*) (ent->entry)) = val;
    return 0;
}

reb_res_t reb_execenv_funccall(reb_strref_t id, reb_res_t arg, reb_execenv_t * ud) {
#define REB_FUNC_START(caps, nocaps, func) if (id.len == (sizeof(caps) - 1)) if (reb_startswith(&id, nocaps, caps, sizeof(caps) - 1)) { REB_DECREF(id.buf, reb_free_strbuf); return func(arg, ud); }
#define REB_FUNC_SET(caps, nocaps, func)
#define REB_FUNC_HELP(str)
#define REB_FUNC_EXAMPLE
#define REB_FUNC_END

#include "gen/reb_funcs.h"

#undef REB_FUNC_START
#undef REB_FUNC_SET
#undef REB_FUNC_HELP
#undef REB_FUNC_EXAMPLE
#undef REB_FUNC_END
    REB_DECREF(id.buf, reb_free_strbuf);
    reb_decref_val(arg);
    return reb_error("Unknown function call.");
}

reb_res_t reb_execenv_sfunccall(reb_strref_t id, reb_res_t idx, reb_res_t arg, reb_execenv_t * ud) {
#define REB_FUNC_START(caps, nocaps, func)
#define REB_FUNC_SET(caps, nocaps, func) if (id.len == (sizeof(caps) - 1)) if (reb_startswith(&id, nocaps, caps, sizeof(caps) - 1)) { REB_DECREF(id.buf, reb_free_strbuf); return func(idx, arg, ud);}
#define REB_FUNC_HELP(str)
#define REB_FUNC_EXAMPLE
#define REB_FUNC_END

#include "gen/reb_funcs.h"
#undef REB_FUNC_START
#undef REB_FUNC_SET
#undef REB_FUNC_HELP
#undef REB_FUNC_EXAMPLE
#undef REB_FUNC_END
    REB_DECREF(id.buf, reb_free_strbuf);
    reb_decref_val(arg);
    reb_decref_val(idx);
    return reb_error("Unknown write function call.");
}

void reb_execenv_init(reb_execenv_t * env) {
    lookuptree_init(&(env->variables));
    env->firstline = 0;
    env->quit_at_prompt = 0;
}

static void reb_execenv_freeval(void * t) {
    reb_res_t * rrt = ((reb_res_t*)t);
    reb_decref_val(*rrt);
    reb_mem_free(rrt);
}
void reb_execenv_free(reb_execenv_t * env) {
    lookuptree_clear(&(env->variables), reb_execenv_freeval);
    reb_exec_lineent_t * line = env->firstline;
    while (line) {
        reb_exec_lineent_t * next = line->next;
        // Free this line
        REB_DECREF(line->line.buf, reb_free_strbuf);
        reb_mem_free(line);
        // --
        line = next;
    }
}
