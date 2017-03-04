// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

// REB commands
// This is the top part of gen/reb_cmds.h
// define REB_CMD_*, include this, undefine,
// or include this without defining REB_CMD_START, and it'll provide the function definitions

#include "../reb_config.h"

#ifndef REB_CMD_START
#ifndef REB_CMDS_H
#define REB_CMDS_H
#define REB_CMD_START(caps, ncaps, func) reb_exec_line_act_t func(reb_strref_t line, reb_execenv_t * env);
#else
#define REB_CMD_START(caps, ncaps, func)
#endif
#define REB_CMD_HELP(str)
#define REB_CMD_EXAMPLE
#define REB_CMD_END
#define REB_CMDS_H_
#endif

REB_CMD_START("PRINT", "print", reb_cmd_print)
REB_CMD_HELP("print [#fileHandle] ... [;]")
REB_CMD_HELP("Writes the given parameters to the screen, followed by a newline.")
REB_CMD_HELP("If a handle ID is given, it is used for output, and the parameters are read after it.")
REB_CMD_HELP("The data is then written there.")
REB_CMD_HELP("If a semicolon is at the end of the statement, rather than concatenate 0, the newline will not be emitted.")
REB_CMD_EXAMPLE
REB_CMD_HELP("print \"Hello World.\"")
REB_CMD_END

REB_CMD_START("RUN", "run", reb_cmd_run)
REB_CMD_HELP("run")
REB_CMD_HELP("GOTO the first line in the program.")
REB_CMD_HELP("Usually used to begin executing a program.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print \"Hello!\"")
REB_CMD_HELP("20 goto 10")
REB_CMD_HELP("run")
REB_CMD_END

REB_CMD_START("GOTO", "goto", reb_cmd_goto)
REB_CMD_HELP("goto <line>")
REB_CMD_HELP("GOTO a line in the program.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 goto 10")
REB_CMD_END

REB_CMD_START("SAVE", "save", reb_cmd_save)
REB_CMD_HELP("save <filename or handle>")
REB_CMD_HELP("Saves the program to a file or file handle.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print \"hello world\"")
REB_CMD_HELP("save \"helloworld.bas\"")
REB_CMD_END

REB_CMD_START("LOAD", "load", reb_cmd_load)
REB_CMD_HELP("load <filename or handle>")
REB_CMD_HELP("Loads a new program from RAM.")
REB_CMD_EXAMPLE
REB_CMD_HELP("load \"helloworld.bas\"")
REB_CMD_END

REB_CMD_START("LIST", "list", reb_cmd_list)
REB_CMD_HELP("list")
REB_CMD_HELP("Lists the code.")
REB_CMD_HELP("An alias for \"save 0\".")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print \"Kittens!\"")
REB_CMD_HELP("list")
REB_CMD_END

REB_CMD_START("IF", "if", reb_cmd_if)
REB_CMD_HELP("if <condition> then <line>")
REB_CMD_HELP("If condition is not 0, then goto line.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print \"What is the answer to life, the universe, and everything?\"")
REB_CMD_HELP("20 input i$")
REB_CMD_HELP("30 if i$ = 42 then 40")
REB_CMD_HELP("31 print \"Wrong.\"")
REB_CMD_HELP("32 goto 10")
REB_CMD_HELP("40 print \"Correct!\"")
REB_CMD_END

REB_CMD_START("OPEN", "open", reb_cmd_open)
REB_CMD_HELP("open <expr> for {input|output|append} as <handle>")
REB_CMD_HELP("open opens a file. Or a device.")
REB_CMD_HELP("Notes on how IO works, if supported, have been left in HELP CLOSE.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 open \"bunnie.txt\" for input as #novenadata")
REB_CMD_HELP("20 input#novenadata line$")
REB_CMD_HELP("21 if line$ = (0,0) then 50")
REB_CMD_HELP("30 print line$")
REB_CMD_HELP("40 goto 20")
REB_CMD_HELP("50 close #novenadata")
REB_CMD_END

REB_CMD_START("INPUT", "input", reb_cmd_input)
REB_CMD_HELP("input [#fileHandle] <variable>")
REB_CMD_HELP("Inputs a line into a variable.")
REB_CMD_HELP("Normally, variables are typeless, but input makes a distinction:")
REB_CMD_HELP("If the ID ends with $, it will accept any input.")
REB_CMD_HELP("Otherwise, the prompt will repeat until a valid (negative or positive) integer is given.")
REB_CMD_HELP("A blank or whitespace line will be treated as 0.")
REB_CMD_HELP("Said integer is written into the variable as an integer.")
REB_CMD_HELP("At the end of the file, it will return the cons (0,0) for lack of a better indicator.")
REB_CMD_HELP("A valid handle ID ('#' followed by the usual ID characters) can be given.")
REB_CMD_HELP("In this particular case, the handle ID is read to get the file number, and reading occurs from there.")
REB_CMD_EXAMPLE
REB_CMD_HELP("input a")
REB_CMD_HELP("input b")
REB_CMD_HELP("print a + b")
REB_CMD_HELP("input#textFile line$")
REB_CMD_END

REB_CMD_START("END", "end", reb_cmd_end)
REB_CMD_HELP("end")
REB_CMD_HELP("If issued directly from the prompt, this will stop Recovery Basic.")
REB_CMD_HELP("Otherwise, it will return to the prompt, without a \"Ready.\" message.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print \"End program?\"")
REB_CMD_HELP("20 input a$")
REB_CMD_HELP("30 if a$ <> \"y\" then 10")
REB_CMD_HELP("40 end")
REB_CMD_END
        
REB_CMD_START("CLOSE", "close", reb_cmd_close)
REB_CMD_HELP("close #<fileHandle>")
REB_CMD_HELP("close closes a file handle.")
REB_CMD_HELP("For lack of a better place, here's some notes on how IO works:")
REB_CMD_HELP("On MINIMAL, all access goes to console and keyboard. Files cannot be opened.")
REB_CMD_HELP("In general, all file handles are actually numbers.")
REB_CMD_HELP("A file handle such as #ABC is actually a variable containing the file number.")
REB_CMD_HELP("On UNIXes, this file number is equal to the file descriptor number.")
REB_CMD_HELP("Furthermore: standard output is 0, standard input is 1, and standard error is 2.")
REB_CMD_EXAMPLE
REB_CMD_HELP("close sys_dup(0)")
REB_CMD_END

REB_CMD_START("REM", "rem", reb_cmd_rem)
REB_CMD_HELP("rem BLAH BLAH BLAH WHO READS THIS SPECIFICATION ANYWAY")
REB_CMD_HELP("Defined to do nothing.")
REB_CMD_HELP("Absolutely nothing. Not even error, under even the weirdest syntax circumstances.")
REB_CMD_HELP("This reaches the very limit of how nothingy nothing can possibly not be.")
REB_CMD_HELP("Potential uses for this are so extremely limited they could fit within 0 <insert unit here>")
REB_CMD_HELP("Since I have no better ideas for an example, have this:")
REB_CMD_EXAMPLE
REB_CMD_HELP("-2 rem Daemon Example")
REB_CMD_HELP("-1 rem The program on 20 to 23 communicates with the program on 4 to 11 via two \"sys_pipe\"s.")
REB_CMD_HELP("0 sys_pipe #params_in #params_pr")
REB_CMD_HELP("1 sys_pipe #results_in #results_pr")
REB_CMD_HELP("2 pid = sys_fork()")
REB_CMD_HELP("3 if pid = 0 then 20")
REB_CMD_HELP("4 input a")
REB_CMD_HELP("5 input b")
REB_CMD_HELP("6 print \"Submitted\"")
REB_CMD_HELP("7 print #params_pr a")
REB_CMD_HELP("8 print #params_pr b")
REB_CMD_HELP("9 input #results_in r$")
REB_CMD_HELP("10 print \"Result:\" + r$")
REB_CMD_HELP("11 goto 4")
REB_CMD_HELP("20 input #params_in a")
REB_CMD_HELP("21 input #params_in b")
REB_CMD_HELP("22 print #results_pr (a + b)")
REB_CMD_HELP("23 goto 20")
REB_CMD_END
