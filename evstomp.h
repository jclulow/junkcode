

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
//#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>

#include <talloc.h>
/* #include <sqlite3.h> */
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#ifndef __EVSTOMP_H__
#define __EVSTOMP_H__

enum evstomp_event_type {
  CONNECTED,
  MESSAGE,
  ERROR
};

struct header;
struct frame;
struct connstate;

void frame_set_header(struct frame *f, char* name, char* value);
const char * frame_get_header(struct frame *f, char* name);
const char * frame_get_body(struct frame *f);

struct evstomp_handle;
struct evstomp_handle *evstomp_init(struct event_base *base, char *hostname, int port);
void evstomp_setcb(struct evstomp_handle *h, void (*func)(enum evstomp_event_type, struct frame *));

#endif /* __EVSTOMP_H__ */

