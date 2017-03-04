// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

REB_CMD_START("SYS_SYNC", "sys_sync", reb_cmd_sys_sync)
REB_CMD_HELP("sys_sync")
REB_CMD_HELP("This is a Unix System Call.")
REB_CMD_HELP("sync flushes the caches of filesystems and hard disks on the computer.")
REB_CMD_HELP("It is very useful when dealing with flash memory devices - Linux aggressively caches for those due to write cycles.")
REB_CMD_HELP("For more documentation, see sync(2) in your manual pages.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 print#savedata health;\" \";zone")
REB_CMD_HELP("20 close #savedata")
REB_CMD_HELP("30 sys_sync")
REB_CMD_END

REB_CMD_START("SYS_PIPE", "sys_pipe", reb_cmd_sys_pipe)
REB_CMD_HELP("sys_pipe #<handleRead> #<handleWrite>")
REB_CMD_HELP("This is a Unix System Call.")
REB_CMD_HELP("Creates two handles. You can write to one, and read from the other.")
REB_CMD_HELP("Note that pipes cannot hold much data - they are much better when combined with sys_fork.")
REB_CMD_HELP("When combined with sys_fork, they can be used for communication between processes.")
REB_CMD_HELP("For more documentation, see pipe(2) in your manual pages, REM, and SYS_FORK(.")
REB_CMD_EXAMPLE
REB_CMD_HELP("10 sys_pipe #r #w")
REB_CMD_HELP("20 print#w \"Hello\"")
REB_CMD_HELP("30 input#r a$")
REB_CMD_HELP("40 print a$")
REB_CMD_END
