# before use: do the file-generation with the standard script, use the same port noted here
REB_PORT=unix

LOCAL_PATH:= $(call my-dir)/../

include $(CLEAR_VARS)

# -- ensure this is copied from the main Makefile verbatim --
BASEOBJ=lookuptree.o reb_cmds.o reb_exec.o reb_exec_cmd.o reb_funcs.o reb_mem.o reb_repl.o reb_util.o
PORTOBJ=ports/$(REB_PORT)/reb_cmds_$(REB_PORT).o ports/$(REB_PORT)/reb_funcs_$(REB_PORT).o ports/$(REB_PORT)/reb_io_$(REB_PORT).o
HEADERS=lookuptree.h reb.h reb_cmds_base.h reb_cmds_foot.h reb_config.h reb_io.h reb_mem.h reb_funcs_base.h reb_funcs_foot.h
PORTHEAD=ports/$(REB_PORT)/reb_cmds_$(REB_PORT).h ports/$(REB_PORT)/reb_funcs_$(REB_PORT).h ports/$(REB_PORT)/stdint.h
GENHEAD=gen/reb_cmds.h gen/reb_funcs.h
# --

LOCAL_MODULE:= recoverybasic
LOCAL_SRC_FILES:= $(BASEOBJ) $(PORTOBJ) $(HEADERS) $(PORTHEAD) $(GENHEAD)
LOCAL_CFLAGS := -std=c99 -DREB_PORT="\"$(REB_PORT)\""

include $(BUILD_EXECUTABLE)
