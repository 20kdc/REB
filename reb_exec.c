// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include <stdlib.h>
#include <string.h>
#include "reb.h"

// A note before you begin reading this file.
// This file, with all it's redundancy,
//  essentially acts as the lexer for Recovery BASIC.
// In particular:
// reb_search_string and reb_search_group perform the actual "lexing".
// reb_search_group is used to split by operators efficiently,
//  while reb_search_string is used for keyword finding where applicable.
// The thing to consider here is that strings and parenthesis are "lock-in" tokens,
//  and are the only "lock-in" tokens in Recovery BASIC's lexical rules.
// By "lock-in", I mean that they are skipped over for search purposes,
//  as they are one massive "token".
// If the cost of tokenization wasn't so high, the following:
// print a;(1 + 2);"twenty-two"
// would lex to:
// print a;'(1 + 2)';'"twenty-two"'
// where the things outlined in single quotes are not searched.
// Thus, '"twenty-two"' does not subtract '"twenty' from 'two"'.

static int chrbr(char chr) {
    if (chr == '(')
        return 1;
    if (chr == ')')
        return -1;
    return 0;
}

// Notes on this: If you wanted to add in-line comments,
// you would, theoretically:
// 1. Add them here (to prevent them being processed)
// 2. Add the in-line comments to reb_trim (to make them essentially whitespace)
// 3. Hope everything works

#define LEXER_VARS int lex_bpos = 0, lex_qpos = 0
#define LEXER_CHK(chr) if (chr == '"') lex_qpos = !lex_qpos; \
                       if (!lex_qpos) \
                       if (!(lex_bpos += chrbr(chr)))

// This is semi-internal. The reference it is given is *transferred to it*.

reb_res_t reb_exec_op(reb_strref_t ref, size_t split, reb_execenv_t * env) {
    // basically: buf[pos + split] == op.
    // execexpr is responsible for isolating these in a BODMAS-friendly way.
    // Is a second opcharacter being used?
    int sc_inuse = 0;
    char sec = 0;
    // First, is that even possible?
    if ((ref.pos + split + 1) < ref.len) {
        // Yes, it is, is it in use?
        sec = ref.buf->buf[ref.pos + split + 1];
        switch (ref.buf->buf[ref.pos + split]) {
            case '>':
                sc_inuse = sec == '=';
                break;
            case '<':
                sc_inuse = (sec == '>');
                if (sec == '=')
                    sc_inuse = 1;
                break;
            default:
                break;
        }
    }
    reb_strref_t subexpr;
    subexpr.buf = ref.buf;
    subexpr.pos = ref.pos;
    subexpr.len = split;
    reb_res_t va = reb_exec_expr(subexpr, env);
    if (va.errorflag) {
        REB_DECREF(ref.buf, reb_free_strbuf);
        return va;
    }
    subexpr.buf = ref.buf;
    subexpr.pos = ref.pos + split + 1 + sc_inuse;
    subexpr.len = ref.len - (split + 1 + sc_inuse);
    reb_res_t vb = reb_exec_expr(subexpr, env);
    if (vb.errorflag) {
        reb_decref_val(va);
        REB_DECREF(ref.buf, reb_free_strbuf);
        return vb;
    }
    char first = ref.buf->buf[ref.pos + split];
    // all required data (op chars, resulting values) now cached
    REB_DECREF(ref.buf, reb_free_strbuf);

    switch (first) {
        case ',':
            return reb_op_cons(va, vb);
        case ';':
            return reb_op_concat(va, vb);
        case '+':
            return reb_op_add(va, vb);
        case '-':
            return reb_op_sub(va, vb);
        case '*':
            return reb_op_mul(va, vb);
        case '/':
            return reb_op_div(va, vb);
        case '%':
            return reb_op_mod(va, vb);
        case '>':
            if (sc_inuse)
                // >= (only possible case here)
                return reb_int(reb_op_compare(va, vb) >= 0);
            return reb_int(reb_op_compare(va, vb) == 1);
        case '<':
            if (sc_inuse) {
                if (sec == '>') {
                    // <>
                    return reb_int(reb_op_compare(va, vb) != 0);
                } else {
                    // <= (only possible case here)
                    int cval = reb_op_compare(va, vb);
                    return reb_int((cval == -1) || (cval == 0));
                }
            }
            return reb_int(reb_op_compare(va, vb) == -1);
        case '=':
            return reb_int(reb_op_compare(va, vb) == 0);
        default:
            return reb_error("Unknown operation, somehow.");
    }
}

// precondition: only a [\r\n\t (\"] char can go before searchstring
// postcondition: only a whitespace char can go after searchstring

int reb_search_string(reb_strref_t expression, char * txt, char * txt2, size_t * res, int precondition, int postcondition) {
    LEXER_VARS;
    size_t txtlen = strlen(txt);
    if (expression.len < txtlen)
        return 0;
    size_t sz = expression.len - (txtlen - 1);
    size_t i = 0;
    if (precondition)
        i++;
    for (; i < sz; i++) {
        char chr = expression.buf->buf[i + expression.pos];
        LEXER_CHK(chr) {
            int ok = 1;
            if (precondition) {
                // only certain chars can go before this
                char prechar = expression.buf->buf[expression.pos + i - 1];
                ok = 0;
                if (REB_CHRWS(prechar))
                    ok = 1;
                if (prechar == ')')
                    ok = 1;
                if (prechar == '\"')
                    ok = 1;
            }
            if (ok)
                if (postcondition)
                    if ((txtlen + i) != expression.len)
                        if (!REB_CHRWS(expression.buf->buf[expression.pos + i + txtlen]))
                            ok = 0;
            if (ok) {
                // Maybe an actual op?
                int sok = 1;
                for (size_t p = 0; p < txtlen; p++) {
                    chr = expression.buf->buf[expression.pos + i + p];
                    if (chr != txt[p])
                        if (chr != txt2[p]) {
                            sok = 0;
                            break;
                        }
                }
                if (sok) {
                    *res = i;
                    return 1;
                }
            }
        }
    }
    return 0;
}
char reb_search_group(char * buf, size_t sz, char * txt, size_t txtlen, size_t * res) {
    char rank = 0;
    LEXER_VARS;
    for (size_t i = 0; i < sz; i++) {
        char chr = *(buf++);
        LEXER_CHK(chr) {
            char * p = memchr(txt, chr, txtlen);
            if (p) {
                p += txtlen;
                char exr = *p;
                if (exr > rank) {
                    *res = i;
                    rank = exr;
                }
            }
        }
    }
    return rank;
}

int reb_startsid(reb_strref_t * other, reb_strref_t * id) {
    char chr = other->buf->buf[other->pos];
    // IDs
    if (reb_chrid(chr, 1)) {
        // First, iterate until len == 0 or NOT a chrid.
        // If the "not a chrid" branch is hit,
        //  then we're dealing with a call, otherwise a var.
        id->buf = other->buf;
        id->len = 1;
        id->pos = other->pos;
        other->len--;
        other->pos++;
        while (other->len > 0) {
            if (!reb_chrid(other->buf->buf[other->pos], 0))
                break;
            other->len--;
            other->pos++;
            id->len++;
        }
        return 1;
    } else {
        return 0;
    }
}

reb_res_t reb_gethandle(reb_strref_t * line, REB_NUMTYPE def, reb_execenv_t * env) {
    reb_res_t defval = reb_int(def);
    reb_strref_t modline, id;
    modline = *line;
    if (reb_startsid(&modline, &id)) {
        if (id.buf->buf[id.pos] == '#') {
            REB_INCREF(id.buf);
            defval = reb_execenv_varget(id, env);
            *line = modline;
        }
    }
    return defval;
}

// Reference not transferred, new reference created to arg in question.
// Used to massively simplify functions that need multiple parameters and use ,-syntax... like they should.
reb_res_t reb_args_retrieve(reb_res_t args, int argnum, reb_execenv_t * env) {
    reb_incref_val(args);
    while (argnum > 0) {
        // creates a reference, so the result can go into args without ref++
        reb_res_t v = reb_func_get_cdr(args, env);
        if (v.errorflag) {
            reb_decref_val(v);
            return reb_error("Could not get argument");
        }
        args = v;
        argnum--;
    }

    // If it's a cons, there are more args available and it has to be CAR'd.
    // If it's not, then it's the arg.
    if (args.vtype == REB_RES_CONS)
        args = reb_func_get_car(args, env);
    // If it returns error, it returns error. Oh well.
    return args;
}

// Again, no reference transfer, new reference created.
reb_res_t reb_args_expect_strref(reb_res_t args, int argnum, reb_execenv_t * env) {
    reb_res_t r = reb_args_retrieve(args, argnum, env);
    if (r.errorflag)
        return r;
    if (r.vtype == REB_RES_STATICSTRING)
        r = reb_destatic(r);
    if (r.vtype != REB_RES_STRINGREF) {
        reb_decref_val(r);
        return reb_error("Argument not string");
    }
    return r;
}

reb_res_t reb_args_expect_num(reb_res_t args, int argnum, reb_execenv_t * env) {
    reb_res_t r = reb_args_retrieve(args, argnum, env);
    if (r.errorflag)
        return r;
    if (r.vtype == REB_RES_NUMBER)
        return r;
    reb_decref_val(r);
    return reb_error("Argument not number");
}

reb_res_t reb_exec_expr(reb_strref_t expression, reb_execenv_t * env) {
    REB_INCREF(expression.buf);
    // Random fact: This function used to have a few CoA references
    // Ok, first scan for operations,
    // in a way that takes "" & () into account by skipping over them.
    // BODMAS is implemented via multiple scanning passes.
    // Operations in order to be considered.
    // Update the "oi <" part, and execop.
    // Also note that a 2char op's first letter
    //  will end up as a 1char op itself,
    //  because of parser weirdness.
    // ADDITIONAL FURTHER NOTES:
    // Since + and - are in the same classification (they get L-t-Red if they're both on the same line),
    // also apparently it's kind of in reverse. Whoops.
    // --Ok, some restructuring later--
    // There, everything is OK now. Modulo is %, not "mod", but TBH that syntax was nuts anyway.
    
    size_t oppos, esize = expression.len;
    char * ebuf = expression.buf->buf + expression.pos;
    char * groups = ",<>=;+-*/%"
                    "9888766555";
    if (reb_search_group(ebuf, esize, groups, 10, &oppos))
        return reb_exec_op(expression, oppos, env);

    // At this point, it's not an operation, which is a good thing.
    // The result comes down to "remove whitespace, check first char".

    reb_trim(&expression);

    // Since it just changed, refresh the useful local pointers.
    esize = expression.len;
    ebuf = expression.buf->buf + expression.pos;

    // First things first, blank string == 0, which is needed because:
    // 1. Negate works because of this
    // 2. It prevents the "function needs a parameter which is completely ignored" syndrome
    //    (which is presumably due to a similar problem)
    if (esize == 0) {
        REB_DECREF(expression.buf, reb_free_strbuf);
        return reb_int(0);
    }

    // And now the actual stuff begins.
    // Arranged in order of likelihood.
    char chr = *ebuf;

    // Strings
    if (chr == '"') {
        if (ebuf[esize - 1] != '\"') {
            REB_DECREF(expression.buf, reb_free_strbuf);
            return reb_error("String does not end with double-quote");
        }
        if (esize == 1) {
            REB_DECREF(expression.buf, reb_free_strbuf);
            return reb_error("String with only one double-quote (unmatched \" or parser error)");
        }
        if (memchr(ebuf + 1, '"', esize - 2)) {
            REB_DECREF(expression.buf, reb_free_strbuf);
            return reb_error("String with embedded double-quotes.");
        }
        // (Note: String implies returning a reference to expression.
        //        So instead of removing the reference we already have,
        //        and giving String a reference, just preserve the reference.
        //        It's not like a reference is anything more than ethereal.
        //        Expect to see this elsewhere in the code, but without the commentary.)
        reb_res_t val;
        val.errorflag = 0;
        val.vtype = REB_RES_STRINGREF;
        expression.len -= 2;
        expression.pos++;
        val.values.v_stringref = expression;
        return val;
    }
    // IDs
    reb_strref_t id;
    if (reb_startsid(&expression, &id)) {
        // Note that the above call did just change reference position
        // (if it returned true)
        // NOTE: id isn't actually counted as a reference for refcounting here.
        if (expression.len != 0) {
            // id/other are now correct, and this is a call w/arg.
            // reb_exec_expr does *not consume a reference*,
            // hence the incref at the top.
            // That reference will instead be consumed by reb_execenv_funccall.
            // refcounting is hard.
            reb_res_t tmpres = reb_exec_expr(expression, env);
            if (tmpres.errorflag)
                return tmpres;
            return reb_execenv_funccall(id, tmpres, env); // This ALSO consumes a reference
        } else {
            // id/other are now correct, and this is a call w/o arg.
            // The reference to expression is given to the env.
            return reb_execenv_varget(id, env);
        }
    }
    // Numbers
    if ((chr >= '0') && (chr <= '9')) {
        // Notably, negative numbers are negated positive numbers.
        // Kind of inefficient but also tricky to deal with,
        // short of some "if the very first character is -" logic,
        // which means doing a trim on operator expressions, which is ALSO inefficient.
        int val = 0;
        while ((chr >= '0') && (chr <= '9')) {
            val *= 10;
            val += chr - '0';

            esize--;
            if (esize == 0) {
                REB_DECREF(expression.buf, reb_free_strbuf);
                return reb_int(val);
            }

            ebuf++;
            chr = *ebuf;
        }
        REB_DECREF(expression.buf, reb_free_strbuf);
        return reb_error("Madness after number?");
    }
    // ()
    if (chr == ')') {
        REB_DECREF(expression.buf, reb_free_strbuf);
        return reb_error("Unmatched )");
    }
    if (chr == '(') {
        if (ebuf[esize - 1] != ')') {
            REB_DECREF(expression.buf, reb_free_strbuf);
            return reb_error("Unmatched (");
        }
        
        reb_strref_t expr2;
        expr2.buf = expression.buf;
        expr2.len = esize - 2; // the above check for ')' implies len >= 2, or characters are quantum
        expr2.pos = expression.pos + 1;
        // run expr2, which uses subpart of expression,
        // then decref to expression
        reb_res_t res = reb_exec_expr(expr2, env);
        REB_DECREF(expression.buf, reb_free_strbuf);
        return res;
    }
    REB_DECREF(expression.buf, reb_free_strbuf);
    return reb_error("Unknown expression format");
}
