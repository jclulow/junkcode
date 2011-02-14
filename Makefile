
LEVENT_DIR = ../ext/libevent-sunos-x86
TALLOC_DIR = ../ext/talloc-sunos-x86
SQLITE_DIR = ../ext/sqlite-sunos-x86
CFLAGS  = -I $(TALLOC_DIR)/include -I $(SQLITE_DIR)/include  -I $(LEVENT_DIR)/include
LDFLAGS = -L $(TALLOC_DIR)/lib -L $(SQLITE_DIR)/lib -L ./lib -R '$$ORIGIN:$$ORIGIN/lib:$$ORIGIN/../lib'

sqlite_sandbox:	sqlite_sandbox.c
	gcc -g $(CFLAGS) $(LDFLAGS) -ltalloc -lsqlite3 $< -o $@

evstomp_example:	evstomp_example.c evstomp.o
	gcc -g $(CFLAGS) $(LDFLAGS) -lnsl -lsocket -ltalloc -levent $< evstomp.o -o $@

evstomp.o:	evstomp.c evstomp.h
	gcc -g -c $(CFLAGS) $< -o $@


clean:
	rm -f *.o evstomp_example sqlite_sandbox

