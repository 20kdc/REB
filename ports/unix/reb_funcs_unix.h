// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

REB_FUNC_START("SYS_FORK", "sys_fork", reb_func_get_sys_fork)
REB_FUNC_HELP("This is a UNIX System Call.")
REB_FUNC_HELP("This function will, when run, create a copy of the current process.")
REB_FUNC_HELP("From the current process's perspective, it will return the ID of the child.")
REB_FUNC_HELP("From the child process's perspective, it will return 0.")
REB_FUNC_HELP("The copy will have a separate copy of variables and line numbers.")
REB_FUNC_HELP("However, it will not have a separate copy of file descriptors.")
REB_FUNC_HELP("Thus, the sys_pipe command can be used before a fork to create a link.")
REB_FUNC_HELP("Note that upon a sys_fork, a flag is set which causes the child to end upon reaching the prompt.")
REB_FUNC_HELP("(This aspect is specific to Recovery Basic.)")
REB_FUNC_HELP("For more information, see fork(2) in your manual pages.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("10 ppid = sys_getpid()")
REB_FUNC_HELP("15 sys_pipe #pipeA #pipeB")
REB_FUNC_HELP("20 pid = sys_fork()")
REB_FUNC_HELP("30 if pid == 0 then 70")
REB_FUNC_HELP("40 print \"I am the parent - my child is: \"; pid")
REB_FUNC_HELP("50 input#pipeB text$")
REB_FUNC_HELP("55 print \"Child said \"; text$")
REB_FUNC_HELP("60 end")
REB_FUNC_HELP("55 print \"I am the child - my parent is \"; ppid")
REB_FUNC_HELP("70 input text$")
REB_FUNC_HELP("80 print#pipeA text$")
REB_FUNC_HELP("90 end")
REB_FUNC_END

REB_FUNC_START("SYS_GETPID", "sys_getpid", reb_func_get_sys_getpid)
REB_FUNC_HELP("This is a UNIX System Call.")
REB_FUNC_HELP("This function simply retrieves the PID of a process.")
REB_FUNC_HELP("For more information, see getpid(2) in your manual pages.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("print \"I'm PID \" ; sys_getpid()")
REB_FUNC_END

// this is one of those functions that won't show up on Windows, for no good reason.
// there are semi-ports, though?
REB_FUNC_START("SYS_READDIR", "sys_readdir", reb_func_get_sys_readdir)
REB_FUNC_HELP("Returns a cons-list of files/subdirectories in a directory (use len, car, cdr).")
REB_FUNC_HELP("Errors if anything goes wrong.")
REB_FUNC_HELP("If the directory is completely empty, just returns 0.")
REB_FUNC_HELP("The nicest way to use this is probably: print str$(sys_readdir(\"whatever\"))")
REB_FUNC_HELP("Which should give a decently formatted list.")
REB_FUNC_EXAMPLE
REB_FUNC_HELP("-1 rem List of files in current directory. Not as efficient as it could be, but more readable.")
REB_FUNC_HELP("0 dir = sys_readdir(\".\")")
REB_FUNC_HELP("1 dirpos = 1")
REB_FUNC_HELP("2 if dirpos > conslen(dirs) then 6")
REB_FUNC_HELP("3 print pick(dirpos, dirs)")
REB_FUNC_HELP("4 dirpos = dirpos + 1")
REB_FUNC_HELP("5 goto 2")
REB_FUNC_HELP("6 end")
REB_FUNC_END
