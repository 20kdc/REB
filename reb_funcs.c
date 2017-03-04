// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "reb.h"
#include <string.h>
#include <assert.h>

// -- Standard --

REB_FUNCS_GET_DEF(reb_func_get_len) {
    if (arg.vtype == REB_RES_STATICSTRING)
        return reb_int(strlen(arg.values.v_staticstring));
    if (arg.vtype == REB_RES_STRINGREF) {
        int32_t len = arg.values.v_stringref.len;
        reb_decref_val(arg);
        return reb_int(len);
    }
    reb_decref_val(arg);
    return reb_error("Cannot len non-string");
}
REB_FUNCS_GET_DEF(reb_func_get_conslen) {
    if (arg.vtype == REB_RES_CONS) {
        reb_cons_t * cons = arg.values.v_cons;
        reb_incref_val(cons->b);
        REB_NUMTYPE num = reb_func_get_conslen(cons->b, env).values.v_number;
        reb_decref_val(arg);
        return reb_int(1 + num);
    }
    reb_decref_val(arg);
    return reb_int(1);
}

REB_FUNCS_GET_DEF(reb_func_get_pick) {
    if (arg.vtype == REB_RES_CONS) {
        reb_cons_t * cons = arg.values.v_cons;
        if (cons->a.vtype != REB_RES_NUMBER) {
            reb_decref_val(arg);
            return reb_error("PICK index must be a number.");
        }
        reb_res_t r = reb_args_retrieve(arg, cons->a.values.v_number, env);
        reb_decref_val(arg);
        return r;
    }
    reb_decref_val(arg);
    return reb_error("PICK takes an index and a cons-list.");
}

REB_FUNCS_GET_DEF(reb_func_get_val) {
    if (arg.vtype == REB_RES_STATICSTRING)
        arg = reb_destatic(arg);
    if (arg.vtype == REB_RES_STRINGREF) {
        REB_NUMTYPE ret = 0;
        reb_strref_t ref = arg.values.v_stringref;
        reb_trim(&ref);
        reb_linenumber(&ref, &ret);
        if (ref.len > 0)
            ret = 0;
        reb_decref_val(arg);
        return reb_int(ret);
    }
    reb_decref_val(arg);
    return reb_error("val must be given string");
}

REB_FUNCS_GET_DEF(reb_func_get_mid_) {
    // First, the args with no cleanup cost.
    // (Note: Apparently I forgot that "no cleanup cost" only applies to the arg itself.)
    //        Also, a new reference is created. So this function used to leak tons of memory.)
    reb_res_t a2 = reb_args_expect_num(arg, 1, env);
    if (a2.errorflag) {
        reb_decref_val(arg);
        return a2;
    }
    reb_res_t a3 = reb_args_expect_num(arg, 2, env);
    if (a3.errorflag) {
        reb_decref_val(arg);
        return a3;
    }
    // Notably, if a2/a3 were not numbers, they would have errored -
    // so it's safe not to decref them.
    reb_res_t a1 = reb_args_expect_strref(arg, 0, env);
    if (a1.errorflag) {
        reb_decref_val(arg);
        return a1;
    }
    // Ok, so now crop values, etc.
    REB_NUMTYPE newpos = a2.values.v_number - 1, newlen = a3.values.v_number;
    if (newpos < 0) {
        newlen += newpos;
        newpos = 0;
    }
    if (newpos > a1.values.v_stringref.len)
        newpos = a1.values.v_stringref.len;
    // now adjust length...
    if (newlen < 0)
        newlen = 0;
    if (newlen > (a1.values.v_stringref.len - newpos))
        newlen = (a1.values.v_stringref.len - newpos);
    a1.values.v_stringref.pos += newpos;
    a1.values.v_stringref.len = newlen;
    // a1 is returned as the modified string reference.
    reb_decref_val(arg);
    return a1;
}

REB_FUNCS_GET_DEF(reb_func_get_str_) {
    return reb_tostring(arg, REB_TOSTRING_PARENS, ", ");
}

REB_FUNCS_GET_DEF(reb_func_get_chr_) {
    if (arg.vtype != REB_RES_NUMBER) {
        reb_decref_val(arg);
        return reb_error("chr$ must be given integer");
    }
    // Number, so never a reference.
    char v = (char) arg.values.v_number;
    return reb_strbuf_val(reb_string(&v, 1));
}

REB_FUNCS_GET_DEF(reb_func_get_asc) {
    if (arg.vtype == REB_RES_STATICSTRING)
        return reb_int(arg.values.v_staticstring[0]);
    if (arg.vtype == REB_RES_STRINGREF) {
        int32_t len = arg.values.v_stringref.len;
        if (len == 0) {
            reb_decref_val(arg);
            return reb_int(0);
        } else {
            unsigned char c = arg.values.v_stringref.buf->buf[arg.values.v_stringref.pos];
            reb_decref_val(arg);
            return reb_int(c);
        }
    }
    reb_decref_val(arg);
    return reb_error("asc must be given string");
}

// -- Esoterics --

REB_FUNCS_GET_DEF(reb_func_get_line_) {
    if (arg.vtype != REB_RES_NUMBER) {
        reb_decref_val(arg);
        return reb_error("line$ must be given integer");
    }
    reb_exec_lineent_t * line = reb_exec_line_env_getline(arg.values.v_number, env);
    if (!line) {
        return reb_static("");
    } else {
        reb_res_t res;
        res.errorflag = 0;
        res.vtype = REB_RES_STRINGREF;
        res.values.v_stringref = line->line;
        reb_incref_val(res);
        return res;
    }
}

REB_FUNCS_SET_DEF(reb_func_set_line_) {
    if (idx.vtype != REB_RES_NUMBER) {
        reb_decref_val(idx);
        reb_decref_val(val);
        return reb_error("line$()= must be given integer index");
    }
    // idx does not need decref because it's an int
    reb_res_t str = reb_tostring(val, REB_TOSTRING_PARENS, " ");
    if (str.errorflag)
        return str;
    
    char * s_buffer;
    size_t s_len;
    
    if (!reb_pullbuf(str, &s_buffer, &s_len)) {
        reb_decref_val(str);
        return reb_error("Output of tostring was not string");
    }
    
    reb_res_t lineres = reb_strbuf_val(reb_string(s_buffer, s_len));
    if (lineres.errorflag) {
        // Ok, forget trying to minimize the footprint, use preexisting resources.
        // (reb_destatic consumes str, hence the incref.)
        reb_incref_val(str);
        lineres = reb_destatic(str);
    }
    if (lineres.errorflag) {
        // There is nothing that can be done
        reb_decref_val(str);
        return reb_error("Out Of Memory minimizing line footprint");
    }
    assert(lineres.vtype == REB_RES_STRINGREF);
    if (reb_exec_line_env_putline(idx.values.v_number, lineres.values.v_stringref, env)) {
        reb_decref_val(str);
        return reb_error("Out Of Memory during putline");
    }
    
    // bump off the reference
    return str;
}

REB_FUNCS_GET_DEF(reb_func_get_var_) {
    arg = reb_destatic(arg);
    if (arg.errorflag)
        return arg;
    if (arg.vtype != REB_RES_STRINGREF) {
        reb_decref_val(arg);
        return reb_error("var$ must be given string");
    }
    return reb_execenv_varget(arg.values.v_stringref, env);
}

REB_FUNCS_SET_DEF(reb_func_set_var_) {
    idx = reb_destatic(idx);
    if (idx.errorflag) {
        reb_decref_val(val);
        return idx;
    }
    if (idx.vtype != REB_RES_STRINGREF) {
        reb_decref_val(idx);
        reb_decref_val(val);
        return reb_error("var$()= must be given string index");
    }
    // varset eats the reference given to it.
    return reb_int(reb_execenv_varset(idx.values.v_stringref, val, env));
}

REB_FUNCS_GET_DEF(reb_func_get_memory) {
    reb_decref_val(arg);
#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
    return reb_int(reb_mem_available());
#else
    return reb_int(-1);
#endif
}

REB_FUNCS_GET_DEF(reb_func_get_car) {
    if (arg.vtype != REB_RES_CONS) {
        reb_decref_val(arg);
        return reb_error("car must be given cons-cell");
    }
    reb_cons_t * cons = arg.values.v_cons;
    reb_res_t res = cons->a;
    reb_incref_val(res);
    reb_decref_val(arg);
    return res;
}

REB_FUNCS_GET_DEF(reb_func_get_cdr) {
    if (arg.vtype != REB_RES_CONS) {
        reb_decref_val(arg);
        return reb_error("cdr must be given cons-cell");
    }
    reb_cons_t * cons = arg.values.v_cons;
    reb_res_t res = cons->b;
    reb_incref_val(res);
    reb_decref_val(arg);
    return res;
}
