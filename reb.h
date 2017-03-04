// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

// REB.
#ifndef REB_H
#define REB_H
#include <stdint.h>
#include "lookuptree.h"
#include "reb_mem.h"
#include "reb_config.h"
#include "reb_io.h"

// === The structures ===

// Number. Does not have to be refcounted, expect omitted refcounting when type is known.
#define REB_RES_NUMBER 1
// the two types of string in REB:
// STATICSTRING: A statically allocated area of memory that will always retain the same value.
//               Also a C-String, so it can't contain null, unlike stringref.
//               Does not have to be refcounted - expect refcounting to be omitted *when the type is known*
//               Thinking of adding the possibility of it being in another address space,
//                but that means I'd have to more or less sink pullbuf in favour of destatic.
//               Course, either way it uses memory on AVR,
//                and flashmem + destatic means at least it's gone when not in use.
// STRINGREF: A reb_strref_t.
#define REB_RES_STATICSTRING 2
#define REB_RES_STRINGREF 3
// A cons.
#define REB_RES_CONS 4

// A buffer of memory.
// Substring operations work on the Java 6 system:
//  char * that is never modified by anything,
//  indexed by position & length, changing the reference.
typedef struct {
    char * buf; // Underlying malloc'd buffer. Might not have null at end.
    size_t len;
    unsigned int rc; // Reference counter
} reb_strbuf_t;

typedef struct {
    reb_strbuf_t * buf;
    size_t pos, len;
} reb_strref_t;

// errorflag: 1 on error. Result is then error details.
typedef struct {
    uint8_t vtype, errorflag;
    union {
        REB_NUMTYPE v_number;
        char * v_staticstring;
        reb_strref_t v_stringref;
        void * v_cons; // reb_cons_t
    } values;
} reb_res_t;

typedef struct {
    reb_res_t a, b; // core results (cannot be ERROR type.)
    unsigned int rc; // Reference counter
} reb_cons_t;

typedef struct {
    REB_NUMTYPE linenum;
    reb_strref_t line;
    void * next;
} reb_exec_lineent_t;

// This holds program state, more or less.
typedef struct {
    // Entries are reb_res_t. If you find them...
    lookuptree_t variables;
    // First line, if any.
    reb_exec_lineent_t * firstline;
    // Do NOT expose the BASIC Prompt. Useful in forked processes.
    int quit_at_prompt;
} reb_execenv_t;

// Run next line, defined in the reb_exec_lineent_t.
// (If in console, this is the same as STOP.)
#define REB_EXECLINEACT_CONTINUE 0
// GOTO a line.
#define REB_EXECLINEACT_GOTO 1
// Stop.
#define REB_EXECLINEACT_STOP 2
// Stop because of an error.
#define REB_EXECLINEACT_STOP_ERR 3

typedef struct {
    uint8_t action;
    REB_NUMTYPE param;
} reb_exec_line_act_t;
// example: REB_DECREF(cons, reb_free_cons)
#define REB_DECREF(str, freer) { (str)->rc--; if (!(str)->rc) freer(str); }
#define REB_INCREF(str) {(str)->rc++; }

// === The functions ===

// ==reb_util.c==

void reb_free_strbuf(reb_strbuf_t * buf);
void reb_free_cons(reb_cons_t * cons);

// ownership transfer is complicated, these help
void reb_incref_val(reb_res_t a);
void reb_decref_val(reb_res_t a);

// caller ownership is not given to callee
void reb_print_val(reb_res_t a, REB_NUMTYPE f);
void reb_print_stringref(reb_strref_t ref, REB_NUMTYPE f);

// ownership transfer: As with reb_print_val, none.
// This gets a buffer/length combination for use in, say, concat.
// Obviously the results of this are tied to the existence of the source buffer,
// so make sure you have a reference on the source.
// 1 means success, 0 means failure.
int reb_pullbuf(reb_res_t a, char ** buf, size_t * len);

// NOTE: caller ownership of given values in reb_op_* and reb_tostring functions is *transferred* to callee
//       Returned value ownership is given to caller.

// add "" around strings.
#define REB_TOSTRING_QUOTES 1
// add () around cons structures.
#define REB_TOSTRING_PARENS 2

// Converts everything into a string in a manner that makes sense.
// Can output staticstring (if input is a staticstring) or stringref.
// incons can be 1 - which shows cons without "()".
// consep is the separator for cons elements. "\t" is good for a Lua print-style
// Currently I have an expression evaluator, and this:
// "Testing", "B"
// becomes this:
// Testing B
// where the "g B" is actually "g\tB".
// Which means the expression evaluator more or less implements PRINT :)
// --- Later. ---
// Note that the reference is given to reb_tostring,
//  and exchanged for the string reference.
reb_res_t reb_tostring(reb_res_t a, int mode, char * consep);

// For functions which can't handle STATICSTRING and remain readable.
// Can safely be used as a = reb_destatic(a);
//  as it's a reference-exchange (like reb_tostring).
reb_res_t reb_destatic(reb_res_t a);

// -Operations-
// Operations eat any references passed to it, and give back what they return

// add except always concatenating no matter what the argument type is
// (used internally by reb_op_add, reb_tostring if it's dealing with cons...
//  suffice to say, having reb_ops around in utils makes life easier)
reb_res_t reb_op_concat(reb_res_t a, reb_res_t b);

reb_res_t reb_op_add(reb_res_t a, reb_res_t b);
reb_res_t reb_op_sub(reb_res_t a, reb_res_t b);
reb_res_t reb_op_mul(reb_res_t a, reb_res_t b);
reb_res_t reb_op_div(reb_res_t a, reb_res_t b);
reb_res_t reb_op_mod(reb_res_t a, reb_res_t b);

reb_res_t reb_op_cons(reb_res_t a, reb_res_t b);

// Returns one of: -2, -1, 0, 1.
// -2 and 0 are used for values which don't have relative comparison.
int reb_op_compare(reb_res_t a, reb_res_t b);

// Simple error reporting.
reb_res_t reb_error(char * err);

// Is a character whitespace?
#define REB_CHRWS(chr) (((chr) == ' ') || ((chr) == '\t') || ((chr) == '\r') || ((chr) == '\n'))

// Is a character usable in an ID?
int reb_chrid(char chr, int first);

// Trims a strref to get rid of whitespace & newlines & etc.
void reb_trim(reb_strref_t * ref);

// Creates a string with a refcount of 1 from a buffer.
// Performs two allocs in the process.
// Can return 0 if out of memory.
reb_strbuf_t * reb_string(char * str, size_t len);
// Should be obvious
reb_res_t reb_int(REB_NUMTYPE i);
// Creates a simple strref for a strbuf. Does not increment refcount.
reb_strref_t reb_strbuf_strref(reb_strbuf_t * buf);
// Wraps reb_strbuf_strref with a result wrapper.
// Will return an "Out Of Memory" error value if passed 0.
reb_res_t reb_strbuf_val(reb_strbuf_t * buf);

reb_res_t reb_static(char * str);

// ==reb_exec.c==

// NOTE: ownership of expr is given to callee
//       Retvalue ownership given to caller.
// This is *really* meant to be called from reb_exec_expr.
// What it does is: 1. expr[split] is an op char. (if not, it will error)
//                  2. Work out which op, split expr into two pieces, and run it.
// That's all it does.
reb_res_t reb_exec_op(reb_strref_t expr, size_t split, reb_execenv_t * env);

// Returns 0 on failure, 1 on success.
// precondition: only a [\r\n\t (\"] char can go before searchstring, and search starts at index 1
// postcondition: only a whitespace char [\r\n\t ] or EOS can go after searchstring
// reb_search_string ignores text in "" and ().
int reb_search_string(reb_strref_t haystack, char * needle, char * needle2, size_t * res, int precondition, int postcondition);
// Rather than search for the first string in another string,
// reb_search_group searches for the first occurrance
// of any of a list of chars in another string.
// Again, it ignores text in "" and ().
// The return value is 0 on failure, but the found char on success.
// (This is extremely performance-critical,
//  and has been optimized as such - hence the char*/size_t convention.)
// The group string should have first "needles" amount of characters,
//  followed by an additional "needles" amount of 'rank' characters.
// The first character of the highest rank found is chosen.
//  (Keep in mind char is signed if setting ranks automatically,
//   I just use characters.)
// Example: reb_search_group("cbc", 3, "abcABC") == 'C' (point 0),
//          as 'c' has a rank of 'C' (highest).
char reb_search_group(char * buf, size_t sz, char * needle, size_t needles, size_t * res);

// Checks if line starts with an id.
// If so, returns 1,
// and line is moved forward while id is set to just contain the id.
// Ownership is not transferred to this function,
// and the copy-to-id does not affect reference counting.
int reb_startsid(reb_strref_t * line, reb_strref_t * id);

// Like reb_startsid, but checking for a handle, then retrieving a variable when done.
// Said variable will either be an errorvalue or a REB_RES_NUMBER.
// As per usual with functions like reb_startsid, no ownership transfer/etc. is performed -
// all references stay as-is.
reb_res_t reb_gethandle(reb_strref_t * line, REB_NUMTYPE def, reb_execenv_t * env);

// Reference not transferred, new reference created to arg in question.
// Used to massively simplify functions that need multiple parameters and use ,-syntax... like they should.
reb_res_t reb_args_retrieve(reb_res_t args, int argnum, reb_execenv_t * env);

// More utility functions for speeding function-writing up
// Again, no reference transfer, new reference created.
// Also note that this forces the result to be a *strref* in particular.
// Otherwise, things get messy...
reb_res_t reb_args_expect_strref(reb_res_t args, int argnum, reb_execenv_t * env);
reb_res_t reb_args_expect_num(reb_res_t args, int argnum, reb_execenv_t * env);

// This does not consume the reference, as you'll probably be using it later anyway.
// However, make sure a reference is being held in case the expression manages to remove it's own reference.
reb_res_t reb_exec_expr(reb_strref_t expression, reb_execenv_t * env);

// ==reb_exec_cmd.c==

// Returns 0 on failure, 1 on success.
// On success, it also moves haystack forward by the needle size, decreasing length.
// (Note: This does not remove whitespace. needlecaps is there because it's simple.)
// "endws" (whitespace or nothing just after last char) was planned,
//  but being able to write "print=1" and having it set "print" to 1 seemed somewhat weird.
// (Further note: As per usual with these things, I've gone and worked to optimize the function.
//                Mixed case is no longer allowed, though you can use either case.)
int reb_startswith(reb_strref_t * haystack, char * needle, char * needlecaps, size_t needlelen);

// If the text starts with a decimal number,
//  read it, modifying haystack's position in the process.
// (Note: This does not remove whitespace)
// Otherwise, do nothing.
// Returns 0 on failure, 1 on success.
int reb_linenumber(reb_strref_t * haystack, REB_NUMTYPE * ret);

// reb_gethandle but with the error handling.
// Returns 1 if you should return a STOP_ERR result & decref stuff,
// but most error handling is done internally.
int reb_gethandle_withchecks(reb_strref_t * line, REB_NUMTYPE * ret, reb_execenv_t * env);

// Returns 0 on IO error, error for other kinds
// Full details are in reb_exec_cmd.c, it's complicated
// See reb_repl.c for an example usage
reb_res_t reb_gets_internal(REB_NUMTYPE f, char * speedbuf, int speedbuf_al, size_t speedbuf_sz, reb_strbuf_t * emergency);

// Returns 0 on IO error, error for other kinds
// Reference given to caller.
reb_res_t reb_gets(REB_NUMTYPE f);

// line is ownership-transferred to callee
// Default value is reb_int32(0), always make sure to safely clean values you're replacing.
// In general, the reb_execenv_varget and reb_execenv_varset functions should work well enough.
// If this returns 0, there wasn't enough memory for a lookuptree_t,
//  or expand was 0 and the variable didn't exist.
lookuptree_t * reb_exec_line_env_getvar(reb_strref_t line, reb_execenv_t * env, int expand);

reb_exec_lineent_t * reb_exec_line_env_getline(REB_NUMTYPE line, reb_execenv_t * env);

// Transfers ownership of line to the created instance of a line.
// If this returns != 0, then the memory couldn't be allocated.
int reb_exec_line_env_putline(REB_NUMTYPE lineno, reb_strref_t line, reb_execenv_t * env);

// Executes line(ownershiptransfer)
reb_exec_line_act_t reb_exec_line(reb_strref_t line, reb_execenv_t * env);


// These receive ownership of everything given to them except env.
// Basically, reb_exec_line parses it, but it's the job of these to work out which function handles a given ID.
// All of these can return error. For varset, that should always be treated as OOM (it's a *very* raw function)
int reb_execenv_varset(reb_strref_t id, reb_res_t var, reb_execenv_t * env);
reb_res_t reb_execenv_funccall(reb_strref_t id, reb_res_t arg, reb_execenv_t * env);
reb_res_t reb_execenv_varget(reb_strref_t id, reb_execenv_t * env);
reb_res_t reb_execenv_sfunccall(reb_strref_t id, reb_res_t idx, reb_res_t arg, reb_execenv_t * env);

void reb_execenv_init(reb_execenv_t * env);
void reb_execenv_free(reb_execenv_t * env);

// == reb_cmds.c ==
#include "gen/reb_cmds.h"
// == reb_funcs.c ==
#include "gen/reb_funcs.h"
// == reb_repl.c ==
// Calls the main REPL.
// Should be wrapped in reb_mem_start and reb_mem_byebye calls.
void reb_repl();

// == Macros Needed By A Handful Of Files ==

#define REB_FUNCS_GET_DEF(id) reb_res_t id(reb_res_t arg, reb_execenv_t * env)
// Note: Return values are ignored & decref'd, except if they're errors.
//       However, it's good to return the value that you *actually* put in post-conversion.
#define REB_FUNCS_SET_DEF(id) reb_res_t id(reb_res_t idx, reb_res_t val, reb_execenv_t * env)

#define REB_CMDS_PREFACE reb_exec_line_act_t result; result.action = REB_EXECLINEACT_CONTINUE; result.param = 0
#define REB_CMDS_DEF(id) reb_exec_line_act_t id(reb_strref_t line, reb_execenv_t * env)

#endif
