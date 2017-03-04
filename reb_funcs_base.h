// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "../reb_config.h"

// REB functions
// This is the top part of gen/reb_funcs.h

// this provides the framework for defined functions.
// (Arrays, if they ever get added, will be an exception.
//  Also arrays will get priority over actual functions.)

#ifndef REB_FUNC_START
#ifndef REB_FUNCS_H
#define REB_FUNCS_H
#define REB_FUNC_START(caps, ncaps, func) reb_res_t func(reb_res_t arg, reb_execenv_t * env);
// Value returned should be value put in post-conversion.
// Obviously the reference is transferred back from callee to caller.
// (Useful if your code converts a value to string and copies it,
//  and you have nowhere to put the reference anyway. But returning errors == more important.)
#define REB_FUNC_SET(caps, ncaps, func) reb_res_t func(reb_res_t idx, reb_res_t val, reb_execenv_t * env);
#else
#define REB_FUNC_START(caps, ncaps, func)
#define REB_FUNC_SET(func)
#endif
#define REB_FUNC_HELP(str)
#define REB_FUNC_EXAMPLE
#define REB_FUNC_END
#define REB_FUNCS_H_
#endif

// -- Standard --

REB_FUNC_START("LEN", "len", reb_func_get_len)
REB_FUNC_HELP("Gets the length of a string, errors for other things")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem prints 5")
REB_FUNC_HELP("print len(\"hello\")")
REB_FUNC_END

REB_FUNC_START("CONSLEN", "conslen", reb_func_get_conslen)
REB_FUNC_HELP("Gets the amount of arguments passed to it, or the length of a conslist")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem prints 3")
REB_FUNC_HELP("print conslen(\"elements\", \"are\", \"fun\")")
REB_FUNC_END

REB_FUNC_START("PICK", "pick", reb_func_get_pick)
REB_FUNC_HELP("Picks one of it's own arguments.")
REB_FUNC_HELP("Useful when dealing with cons lists, as:")
REB_FUNC_HELP(" cons = (\"A\", \"B\")")
REB_FUNC_HELP(" print conspick(1, cons)")
REB_FUNC_HELP("is the same as:")
REB_FUNC_HELP(" print conspick(1, \"A\", \"B\")")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem prints \"elements\"")
REB_FUNC_HELP("print conspick(1, \"elements\", \"are\", \"fun\")")
REB_FUNC_END

REB_FUNC_START("VAL", "val", reb_func_get_val)
REB_FUNC_HELP("Turns a string into a number, returning 0 if the string cannot be converted to a number.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem returns 10")
REB_FUNC_HELP("print val(\"5\") + val(\"5\")")
REB_FUNC_END

REB_FUNC_START("MID$", "mid$", reb_func_get_mid_)
REB_FUNC_HELP("Takes 3 parameters: (str, start, len)")
REB_FUNC_HELP("Gets a substring of a value.")
REB_FUNC_HELP("Indexes start at 1, and are inclusive - (\"ABC\", 2, 1) will give you \"B\".")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("print mid$(\"are you a cat\", 11, 3)")
REB_FUNC_END

REB_FUNC_START("STR$", "str$", reb_func_get_str_)
REB_FUNC_HELP("Converts a value into a string.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem Print converts to string anyway, but this way's more memory-wasteful, yay!")
REB_FUNC_HELP("print str$(28)")
REB_FUNC_END

REB_FUNC_START("CHR$", "chr$", reb_func_get_chr_)
REB_FUNC_HELP("Creates a string from a byte.")
REB_FUNC_HELP("For example, 65 is \"A\".")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("print chr$(65)")
REB_FUNC_END

REB_FUNC_START("ASC", "asc", reb_func_get_asc)
REB_FUNC_HELP("Gets the first byte of a string (a value between 0 and 255).")
REB_FUNC_HELP("For example, the letter \"A\" is 65.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("print asc(\"A\");\"=65\"")
REB_FUNC_END

// -- Esoterics --
        
REB_FUNC_START("LINE$", "line$", reb_func_get_line_)
REB_FUNC_HELP("Retrieves a line from the program.")
REB_FUNC_HELP("If the line does not exist, \"\" will be returned.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("10 print line$(30)")
REB_FUNC_HELP("20 end")
REB_FUNC_HELP("30 Hello world.")
REB_FUNC_HELP("(Lines are always trimmed before being put into memory.)")
REB_FUNC_END

REB_FUNC_SET("LINE$", "line$", reb_func_set_line_)
REB_FUNC_HELP("Writes a line into the program.")
REB_FUNC_HELP("To put it bluntly: This allows self-modification.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("10 line$(20) = \"a = a + 1\"")
REB_FUNC_HELP("15 a = 0")
REB_FUNC_HELP("20 a = a - 1")
REB_FUNC_HELP("30 print a")
REB_FUNC_HELP("(The result will be 1, not -1 as would be expected.)")
REB_FUNC_END

REB_FUNC_START("VAR$", "var$", reb_func_get_var_)
REB_FUNC_HELP("Allows reading from a variable named by a string.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("10 a$ = \"Hello World.\"")
REB_FUNC_HELP("15 b$ = \"a$\"")
REB_FUNC_HELP("20 print var$(b$)")
REB_FUNC_END

REB_FUNC_SET("VAR$", "var$", reb_func_set_var_)
REB_FUNC_HELP("Allows writing to a variable named by a string.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("10 b$ = \"a$\"")
REB_FUNC_HELP("15 var$(b$) = \"Hello World.\"")
REB_FUNC_HELP("20 print a$")
REB_FUNC_END

REB_FUNC_START("MEMORY", "memory", reb_func_get_memory)
REB_FUNC_HELP("Ignores any arguments given to it.")
REB_FUNC_HELP("Instead, it returns the amount of memory available (in bytes), or -1 if unknown.")
REB_FUNC_HELP("(Memory cannot be traced unless using the custom allocator.)")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("print (memory() / 1024) ; \" KiB of RAM available!\"")
REB_FUNC_END
        
REB_FUNC_START("CAR", "car", reb_func_get_car)
REB_FUNC_HELP("From a cons cell / right-nested pair list (both created using the ',' operator), get element A.")
REB_FUNC_HELP("Note on cons (pronounced con-ss ) cells:")
REB_FUNC_HELP("Cons cells are an imported aspect of LISP.")
REB_FUNC_HELP("They should, by most standards, have no use here.")
REB_FUNC_HELP("However, they are relatively useful as a way of implementing multiple-argument functions.")
REB_FUNC_HELP("The ',' operator creates a pair which holds the value on the left as CAR and the value on the right as CDR.")
REB_FUNC_HELP("Putting ',' directly next to each other creates a list where CARs are elements, and CDRs are either more pairs or the final value.")
REB_FUNC_HELP("This is by no means perfect, but there's no \"nil\" value, and it would be preferable if the syntax was consistent.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem prints Magic")
REB_FUNC_HELP("print car((\"Magic\",12))")
REB_FUNC_END

REB_FUNC_START("CDR", "cdr", reb_func_get_cdr)
REB_FUNC_HELP("From a cons cell / right-nested pair list (both created using the ',' operator), get element B.")
REB_FUNC_HELP("Notes on cons cells can be seen in CDR(.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("rem prints 12")
REB_FUNC_HELP("print car((\"Magic\",12))")
REB_FUNC_END
