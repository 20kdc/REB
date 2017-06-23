// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "reb.h"

// Useful for finding memory lack-of-frees/too-many-frees
// #define REB_UTIL_DEBUG_FREES

#ifdef REB_UTIL_DEBUG_FREES
#include <stdio.h>
#endif

void reb_free_strbuf(reb_strbuf_t * buf) {
#ifdef REB_UTIL_DEBUG_FREES
    fputs("A strbuf has been freed, content \"", stdout);
    fwrite(buf->buf, buf->len, 1, stdout);
    fputs("\"\n", stdout);
#endif
    // if the len == 0, then buf->buf must == 0.
    if (buf->buf)
        reb_mem_free(buf->buf);
    reb_mem_free(buf);
}

void reb_free_cons(reb_cons_t * cons) {
#ifdef REB_UTIL_DEBUG_FREES
    puts("A cons has been freed!");
#endif
    reb_decref_val(cons->a);
    reb_decref_val(cons->b);
    reb_mem_free(cons);
}

void reb_incref_val(reb_res_t a) {
    if (a.vtype == REB_RES_CONS)
        REB_INCREF((reb_cons_t*) a.values.v_cons);
    if (a.vtype == REB_RES_STRINGREF)
        REB_INCREF(a.values.v_stringref.buf);
}

void reb_decref_val(reb_res_t a) {
    if (a.vtype == REB_RES_CONS)
        REB_DECREF((reb_cons_t*) a.values.v_cons, reb_free_cons);
    if (a.vtype == REB_RES_STRINGREF)
        REB_DECREF(a.values.v_stringref.buf, reb_free_strbuf);
}

static void reb_numbuf16(char * tmpbuf2, REB_NUMTYPE n) {
    char tmpbuf[16];
    char * ptr = tmpbuf + 14;
    char * ptr2 = tmpbuf2;
    int negative = 0;
    if (n == 0) {
        tmpbuf2[0] = '0';
        tmpbuf2[1] = 0;
        return;
    }
    tmpbuf[15] = 0;
    if (n < 0)
        negative = 1;
    while (n != 0) {
        int digit = n % 10;
        if (digit != 0)
            if (negative)
                digit = -digit;
        *ptr = '0' + digit;
        n /= 10;
        ptr--;
        if (ptr == tmpbuf)
            break;
    }
    if (negative) {
        *ptr = '-';
        ptr--;
    }
    while (ptr != (tmpbuf + 15)) {
        ptr++;
        *ptr2 = *ptr;
        ptr2++;
    }
}

void reb_print_val(reb_res_t a, REB_NUMTYPE f) {
    if (a.errorflag)
        reb_io_puts(f, "Error: ");
    if (a.vtype == REB_RES_NUMBER) {
        // Cheat: DON'T use tostring, it's a waste of heap -
        // Instead use a tiny bit of stack, that was going to be used anyway,
        //  by tostring.
        char tmpbuf[16];
        reb_numbuf16(tmpbuf, a.values.v_number);
        reb_io_write(f, tmpbuf, strlen(tmpbuf));
        return;
    }
    // The way this works:
    // tostring will return a value which will be consumed.
    // It will either return the value it received (in which case this consumes it),
    // or consume the value it received after making a new value.
    // Either way, the value of a will be consumed --
    //  thus, increment beforehand, since this function doesn't receive ownership.
    reb_incref_val(a);
    reb_res_t str = reb_tostring(a, 0, "\t");
    char * printbuf;
    size_t printlen;
    reb_pullbuf(str, &printbuf, &printlen);
    reb_io_write(f, printbuf, printlen);
    reb_decref_val(str);
}

void reb_print_stringref(reb_strref_t ref, REB_NUMTYPE f) {
    reb_io_write(f, ref.buf->buf + ref.pos, ref.len);
}

int reb_pullbuf(reb_res_t a, char ** buf, size_t * len) {
    if (a.vtype == REB_RES_STATICSTRING) {
        *buf = a.values.v_staticstring;
        *len = strlen(a.values.v_staticstring);
        return 1;
    }
    if (a.vtype == REB_RES_STRINGREF) {
        *buf = a.values.v_stringref.buf->buf + a.values.v_stringref.pos;
        *len = a.values.v_stringref.len;
        return 1;
    }
    return 0;
}

reb_res_t reb_tostring(reb_res_t a, int tostringmode, char * consep) {
    if ((a.vtype == REB_RES_STATICSTRING) || (a.vtype == REB_RES_STRINGREF)) {
        if (tostringmode & REB_TOSTRING_QUOTES) {
            a = reb_op_concat(reb_static("\""), a);
            if (a.errorflag)
                return a;
            a = reb_op_concat(a, reb_static("\""));
            // if it's an error, just return it anyway
        }
        return a;
    }
    if (a.vtype == REB_RES_NUMBER) {
        // int doesn't need deallocation
        char tmpbuf[16];
        reb_numbuf16(tmpbuf, a.values.v_number);
        return reb_strbuf_val(reb_string(tmpbuf, strlen(tmpbuf)));
    }
    if (a.vtype == REB_RES_CONS) {
        // now THIS is complicated...
        reb_cons_t * cons = a.values.v_cons;
        // generate references to A/B values which will get consumed by the tostrings
        reb_res_t va = cons->a;
        reb_incref_val(va);
        reb_res_t vb = cons->b;
        reb_incref_val(vb);

        // for a standard "next element on right" list of (1 (2 (3 4))):
        // (1 2 3 4)
        // for a nonstandard binary tree:
        // ((1 2) 3 4)

        // reb_string generates a reference, concat consumes it, and references were created above for a/b values.
        // (since tostring will consume the a/b values, or return them verbatim and let concat consume them)
        reb_res_t str;
        // note: vA is always incons 0, so it's always going to be wrapped in (),
        //       so it should always use "standard" syntax: ", " for consep
#define REB_UTIL_TOSTRING_CHK if (str.errorflag) {reb_decref_val(a); return str;}
        str = reb_tostring(va, REB_TOSTRING_PARENS | REB_TOSTRING_QUOTES, ", ");
        REB_UTIL_TOSTRING_CHK;
        if (tostringmode & REB_TOSTRING_PARENS) {
            str = reb_op_concat(reb_static("("), str);
            REB_UTIL_TOSTRING_CHK;
        }
        str = reb_op_concat(str, reb_static(consep));
        REB_UTIL_TOSTRING_CHK;
        str = reb_op_concat(str, reb_tostring(vb, REB_TOSTRING_QUOTES, consep));
        REB_UTIL_TOSTRING_CHK;
        if (tostringmode & REB_TOSTRING_PARENS) {
            str = reb_op_concat(str, reb_static(")"));
            REB_UTIL_TOSTRING_CHK;
        }
#undef REB_UTIL_TOSTRING_CHK
        // String is generated with refcount of 1, now to clean up the cons,
        reb_decref_val(a);
        return str;
    }
    reb_decref_val(a);
    return reb_error("Cannot convert unknown type to string");
}

reb_res_t reb_destatic(reb_res_t a) {
    if (a.vtype != REB_RES_STATICSTRING)
        return a;
    // staticstring doesn't need consume
    return reb_strbuf_val(reb_string(a.values.v_staticstring, strlen(a.values.v_staticstring)));
}

reb_res_t reb_op_concat(reb_res_t a, reb_res_t b) {
    // convert both args to STRINGREF or STATICSTRING
    a = reb_tostring(a, REB_TOSTRING_PARENS, ", ");
    if (a.errorflag) {
        reb_decref_val(b);
        return a;
    }
    b = reb_tostring(b, REB_TOSTRING_PARENS, ", ");
    if (b.errorflag) {
        reb_decref_val(a);
        return b;
    }
    // concat parameters for stage 2
    char * a_buf, * b_buf;
    size_t a_len, b_len;

    if (!reb_pullbuf(a, &a_buf, &a_len)) {
        reb_decref_val(a);
        reb_decref_val(b);
        return reb_error("Could not pull buffer from A");
    }
    if (!reb_pullbuf(b, &b_buf, &b_len)) {
        reb_decref_val(a);
        reb_decref_val(b);
        return reb_error("Could not pull buffer from B");
    }

    // Don't waste memory on empty concats.

    if (a_len == 0) {
        reb_decref_val(a);
        return b; // b's reference gets passed out.
    }
    if (b_len == 0) {
        reb_decref_val(b);
        return a; // a's reference gets passed out.
    }

    // stage 2:

    reb_strbuf_t * concat = reb_mem_alloc(sizeof(reb_strbuf_t));
    if (!concat) {
        reb_decref_val(a);
        reb_decref_val(b);
        return reb_error("Not enough memory for strbuf_t");
    }
    char * conbuf = 0;
    if ((a_len + b_len) != 0) {
        conbuf = reb_mem_alloc(a_len + b_len);
        if (!conbuf) {
            reb_mem_free(concat);
            reb_decref_val(a);
            reb_decref_val(b);
            return reb_error("Not enough memory for concat");
        }
        memcpy(conbuf, a_buf, a_len);
        memcpy(conbuf + a_len, b_buf, b_len);
    }
    concat->buf = conbuf;
    concat->len = a_len + b_len;
    concat->rc = 1;

    reb_decref_val(a);
    reb_decref_val(b);
    return reb_strbuf_val(concat);
}

reb_res_t reb_op_add(reb_res_t a, reb_res_t b) {
    int concat = 0;
    if (a.vtype != REB_RES_NUMBER)
        concat = 1;
    if (b.vtype != REB_RES_NUMBER)
        concat = 1;
    if (concat)
        // default behavior: concat
        return reb_op_concat(a, b);
    // int doesn't need deallocating.
    // Only avoid deallocs when value type known,
    //  and make sure to mark with (x) doesn't need deallocating / similar
    reb_res_t intv;
    intv.errorflag = 0;
    intv.vtype = REB_RES_NUMBER;
    intv.values.v_number = a.values.v_number + b.values.v_number;
    return intv;
}

#define REB_OP_MATHS_REQNUM {int concat = 0; if (a.vtype != REB_RES_NUMBER) concat = 1; if (b.vtype != REB_RES_NUMBER) concat = 1; if (concat) {reb_decref_val(a); reb_decref_val(b); return reb_error("Incorrect use of maths op.");}}
reb_res_t reb_op_sub(reb_res_t a, reb_res_t b) {
    REB_OP_MATHS_REQNUM;
    reb_res_t intv;
    intv.errorflag = 0;
    intv.vtype = REB_RES_NUMBER;
    intv.values.v_number = a.values.v_number - b.values.v_number;
    return intv;
}
reb_res_t reb_op_mul(reb_res_t a, reb_res_t b) {
    REB_OP_MATHS_REQNUM;
    reb_res_t intv;
    intv.errorflag = 0;
    intv.vtype = REB_RES_NUMBER;
    intv.values.v_number = a.values.v_number * b.values.v_number;
    return intv;
}
reb_res_t reb_op_div(reb_res_t a, reb_res_t b) {
    REB_OP_MATHS_REQNUM;
    reb_res_t intv;
    intv.errorflag = 0;
    intv.vtype = REB_RES_NUMBER;
    intv.values.v_number = a.values.v_number / b.values.v_number;
    return intv;
}
reb_res_t reb_op_mod(reb_res_t a, reb_res_t b) {
    REB_OP_MATHS_REQNUM;
    reb_res_t intv;
    intv.errorflag = 0;
    intv.vtype = REB_RES_NUMBER;
    intv.values.v_number = a.values.v_number % b.values.v_number;
    return intv;
}
#undef REB_OP_MATHS_REQNUM

reb_res_t reb_op_cons(reb_res_t a, reb_res_t b) {
    // references to vals a & b transferred to cons
    reb_cons_t * cons = reb_mem_alloc(sizeof (reb_cons_t));
    if (!cons) {
        reb_decref_val(a);
        reb_decref_val(b);
        return reb_error("Out Of Memory for cons");
    }
    cons->a = a;
    cons->b = b;
    cons->rc = 1;
    reb_res_t res;
    res.errorflag = 0;
    res.values.v_cons = cons;
    res.vtype = REB_RES_CONS;
    return res;
}

// Returns one of: -2, -1, 0, 1.
// -2 and 0 are used for values which don't have relative comparison.

int reb_op_compare(reb_res_t a, reb_res_t b) {
    a = reb_destatic(a);
    b = reb_destatic(b);
    if (a.vtype != b.vtype) {
        reb_decref_val(a);
        reb_decref_val(b);
        return -2;
    }
    int cv = -2;
    int tmp = 0;
    reb_res_t va, vb, vc, vd;
    switch (a.vtype) {
        case REB_RES_NUMBER:
            cv = 0;
            if (a.values.v_number < b.values.v_number)
                cv = -1;
            if (a.values.v_number > b.values.v_number)
                cv = 1;
            break;
        case REB_RES_CONS:
            va = ((reb_cons_t*) a.values.v_cons)->a;
            reb_incref_val(va);
            vb = ((reb_cons_t*) a.values.v_cons)->b;
            reb_incref_val(vb);
            vc = ((reb_cons_t*) b.values.v_cons)->a;
            reb_incref_val(vc);
            vd = ((reb_cons_t*) b.values.v_cons)->b;
            reb_incref_val(vd);
            tmp = (reb_op_compare(va, vc) == 0);
            if (reb_op_compare(vb, vd) != 0)
                tmp = 0;
            if (tmp)
                cv = 0;
            break;
        case REB_RES_STRINGREF:
            if (a.values.v_stringref.len == b.values.v_stringref.len)
                if (!memcmp(a.values.v_stringref.buf->buf + a.values.v_stringref.pos, b.values.v_stringref.buf->buf + b.values.v_stringref.pos, a.values.v_stringref.len))
                    cv = 0;
            break;
        default:
            break;
    }
    reb_decref_val(a);
    reb_decref_val(b);
    return cv;
}

// Simple error reporting.

reb_res_t reb_error(char * err) {
    reb_res_t res = reb_static(err);
    res.errorflag = 1;
    return res;
}

int reb_chrid(char chr, int first) {
    if ((chr >= 'a') && (chr <= 'z'))
        return 1;
    if ((chr >= 'A') && (chr <= 'Z'))
        return 1;
    if ((chr >= '0') && (chr <= '9'))
        return !first;
    if (chr == '_')
        return 1;
    // Has special meaning if used at the end of the ID part of an input -
    //  without it, input will require a number.
    if (chr == '$')
        return 1;
    // Has special meaning as part of:
    //  input#fileHandle
    // and
    //  print#fileHandle
    // So, not much of an actual meaning, but the appearance of one -
    // it represents a handle.
    // inputNotAHandle won't work - just input#AHandle.
    if (chr == '#')
        return 1;
    return 0;
}

// Squeeze out whitespace.

void reb_trim(reb_strref_t * ref) {
    size_t pos = ref->pos, len = ref->len;
    char * buf = ref->buf->buf + pos;
    while (len > 0) {
        if (REB_CHRWS(*buf)) {
            // first char is WS
            pos++;
            buf++;
            len--;
            continue;
        }
        while (len > 0) {
            if (REB_CHRWS(buf[len - 1])) {
                // last char is WS
                len--;
                continue;
            }
            break;
        }
        // if it gets to here nothing got done and squeeze is over
        break;
    }
    ref->pos = pos;
    ref->len = len;
}

reb_strbuf_t * reb_string(char * str, size_t len) {
    reb_strbuf_t * ra = reb_mem_alloc(sizeof(reb_strbuf_t));
    if (!ra)
        return 0;
    char * rb = 0;
    if (len != 0) {
        rb = reb_mem_alloc(len);
        if (!rb) {
            reb_mem_free(ra);
            return 0;
        }
    }
    memcpy(rb, str, len);
    ra->rc = 1;
    ra->buf = rb;
    ra->len = len;
    return ra;
}

reb_res_t reb_int(REB_NUMTYPE i) {
    reb_res_t r;
    r.errorflag = 0;
    r.values.v_number = i;
    r.vtype = REB_RES_NUMBER;
    return r;
}

reb_strref_t reb_strbuf_strref(reb_strbuf_t * buf) {
    reb_strref_t ref;
    ref.buf = buf;
    ref.pos = 0;
    ref.len = buf->len;
    return ref;
}

reb_res_t reb_strbuf_val(reb_strbuf_t * buf) {
    if (buf == 0)
        return reb_error("Out Of Memory");
    reb_res_t res;
    res.errorflag = 0;
    res.vtype = REB_RES_STRINGREF;
    res.values.v_stringref = reb_strbuf_strref(buf);
    return res;
}

reb_res_t reb_static(char * str) {
    reb_res_t res;
    res.errorflag = 0;
    res.vtype = REB_RES_STATICSTRING;
    res.values.v_staticstring = str;
    return res;
}
