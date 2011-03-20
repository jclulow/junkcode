
CC = gcc
LEVENT_DIR = ../ext/libevent-sunos-x86
TALLOC_DIR = ../ext/talloc-sunos-x86
SQLITE_DIR = ../ext/sqlite-sunos-x86
DEBUG_FLAGS = -g -Wall
CFLAGS  = -I $(TALLOC_DIR)/include -I $(SQLITE_DIR)/include  -I $(LEVENT_DIR)/include
LDFLAGS = -L $(TALLOC_DIR)/lib -L $(SQLITE_DIR)/lib -L ./lib -R '$$ORIGIN:$$ORIGIN/lib:$$ORIGIN/../lib'

sqlite_sandbox:	sqlite_sandbox.c
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) $(LDFLAGS) -ltalloc -lsqlite3 $< -o $@

evstomp_example:	evstomp_example.c evstomp.o jmcstr.o
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) $(LDFLAGS) -lnsl -lsocket -ltalloc -levent $< evstomp.o jmcstr.o -o $@

evstomp.o:	evstomp.c evstomp.h
	$(CC) $(DEBUG_FLAGS) -c $(CFLAGS) $< -o $@

jmcstr.o:	jmcstr.c jmcstr.h
	$(CC) $(DEBUG_FLAGS) -c $(CFLAGS) $< -o $@

jmcstr_example:	jmcstr_example.c jmcstr.o
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) $(LDFLAGS) -ltalloc $< jmcstr.o -o $@


clean:
	rm -f *.o evstomp_example sqlite_sandbox jmcstr_example

