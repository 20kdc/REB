// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include <string.h>

#include "reb.h"

void reb_repl() {
    // Firstly, before things even start up, get some memory on stack.
    // This memory will be used in case of a complete out-of-memory situation,
    //  so the command interpreter will still run (assuming the user doesn't worsen things)
    char emergency_buf[32];
    reb_strbuf_t emergency_strbuf;
    emergency_strbuf.buf = emergency_buf;
    emergency_strbuf.len = 32;
    emergency_strbuf.rc = 1; // Must never hit 0, as this is off heap!!!
    // Continuing.

    reb_io_puts(REB_IO_CONSOLE, "Recovery Basic on " REB_PORT " started.");
#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
    reb_print_val(reb_int(reb_mem_available()), REB_IO_CONSOLE);
    reb_io_puts(REB_IO_CONSOLE, " bytes available.");
#else
    reb_io_puts(REB_IO_CONSOLE, "Memory available unknown.");
#endif
    reb_execenv_t env;
    reb_execenv_init(&env);
    reb_io_puts(REB_IO_CONSOLE, "Ready.");
    while (!env.quit_at_prompt) {
        reb_res_t line;
        char * linehbuf = reb_mem_alloc(255);
        // Plan A. Allocate bytes from heap, use that for the input line.
        // (This appears very fast, as no allocation occurs after input,
        //   and no copies are required.)
        if (linehbuf) {
            line = reb_gets_internal(REB_IO_KEYBOARD, linehbuf, 1, 255, 0);
        } else {
            // Plan B. Likely in a dangerous state, can the emergency buffer be used?
            if (emergency_strbuf.rc == 1) {
                reb_io_puts(REB_IO_CONSOLE, "Warning: Extremely low on memory, but you should have 32 text chars.");
                line = reb_gets_internal(REB_IO_KEYBOARD, emergency_buf, 0, 32, &emergency_strbuf);
            } else {
                // Plan C, see if the user can write a command to deallocate in ~4 bytes.
                reb_io_puts(REB_IO_CONSOLE, "Warning: Probably doomed.");
                line = reb_gets(REB_IO_KEYBOARD);
            }
        }
        //reb_io_puts(REB_IO_CONSOLE, "---"); // Makes debugging leaks easier.
        if (line.errorflag) {
            reb_print_val(line, REB_IO_CONSOLE);
            reb_decref_val(line);
            reb_io_puts(REB_IO_CONSOLE, "\nLine not executed - Ready.");
            continue;
        } else if (line.vtype == REB_RES_NUMBER) {
            // EOF
            break;
        } else if (line.values.v_stringref.buf == &emergency_strbuf) {
            // This will have created a reference.
            reb_io_puts(REB_IO_CONSOLE, "WARNING: Emergency buffer now in use (there's only one).");
            reb_io_puts(REB_IO_CONSOLE, "If a reference is created, you will likely have to restart Recovery BASIC.");
        }

        reb_exec_line_act_t act = reb_exec_line(line.values.v_stringref, &env);
        
        if (act.action == REB_EXECLINEACT_GOTO) {
            reb_exec_lineent_t * lineent = reb_exec_line_env_getline(act.param, &env);
            // Last line executed
            REB_NUMTYPE lastline = -1;
            while ((act.action != REB_EXECLINEACT_STOP) && (act.action != REB_EXECLINEACT_STOP_ERR)) {
                if (!lineent) {
                    reb_print_val(reb_int(act.param), REB_IO_CONSOLE);
                    reb_io_puts(REB_IO_CONSOLE, " not a valid line.");
                    break;
                } else {
                    lastline = lineent->linenum;
                    REB_INCREF(lineent->line.buf);
                    act = reb_exec_line(lineent->line, &env);
                    if (act.action == REB_EXECLINEACT_CONTINUE) {
                        if (lineent->next) {
                            lineent = lineent->next;
                        } else {
                            break;
                        }
                    } else if (act.action == REB_EXECLINEACT_GOTO) {
                        lineent = reb_exec_line_env_getline(act.param, &env);
                        if (!lineent) {
                            reb_print_val(reb_int(act.param), REB_IO_CONSOLE);
                            reb_io_puts(REB_IO_CONSOLE, " not a valid line - ending.");
                            break;
                        }
                    }
                }
            }
            if (act.action != REB_EXECLINEACT_STOP_ERR) {
                // If not an error, and END didn't do this, write "Ready."
                if (act.action != REB_EXECLINEACT_STOP)
                    reb_io_puts(REB_IO_CONSOLE, "Ready.");
            } else {
                reb_print_val(reb_int(lineent->linenum), REB_IO_CONSOLE);
                reb_io_puts(REB_IO_CONSOLE, " line error. Ready...");
            }
        } else {
            if (act.action == REB_EXECLINEACT_STOP)
                break;
            if (act.action != REB_EXECLINEACT_STOP_ERR) {
                // If not an error, write "Ready."
                reb_io_puts(REB_IO_CONSOLE, "Ready.");
            } else {
                reb_io_puts(REB_IO_CONSOLE, "Error at prompt. Ready.");
            }
        }
    }
    reb_execenv_free(&env);
}
