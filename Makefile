
LEVENT_DIR = ../ext/libevent-sunos-x86
TALLOC_DIR = ../ext/talloc-sunos-x86
SQLITE_DIR = ../ext/sqlite-sunos-x86
CFLAGS  = -I $(TALLOC_DIR)/include -I $(SQLITE_DIR)/include  -I $(LEVENT_DIR)/include
LDFLAGS = -L $(TALLOC_DIR)/lib -L $(SQLITE_DIR)/lib -L ./lib -R '$$ORIGIN:$$ORIGIN/lib:$$ORIGIN/../lib'

s1:	s1.c
	gcc -g $(CFLAGS) $(LDFLAGS) -ltalloc -lsqlite3 $< -o $@

t1:	t1.c
	gcc -g $(CFLAGS) $(LDFLAGS) -ltalloc $< -o $@

e1:	e1.c
	gcc -g $(CFLAGS) $(LDFLAGS) -ltalloc -levent $< -o $@

n1:	n1.c evstomp.o
	gcc -g $(CFLAGS) $(LDFLAGS) -lnsl -lsocket -ltalloc -levent $< evstomp.o -o $@

evstomp.o:	evstomp.c evstomp.h
	gcc -g -c $(CFLAGS) $< -o $@


clean:
	rm -f t1 s1 e1 n1 *.o

