

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

#include "evstomp.h"

#define MAX_SIGINTS 1

int xx = 0;
int sigintcount = 0;

void
sample3(evutil_socket_t fd, short event, void *arg)
{
  fprintf(stderr, "SIGINT NUMBER %d\n", ++sigintcount);
  if (sigintcount >= MAX_SIGINTS) {
    exit(0);
  }
}

void
h_usr1(evutil_socket_t fd, short event, void *arg)
{
  fprintf(stderr, "\n-- USR1 HANDLER --------------\n");
  talloc_report_full(NULL, stderr);
  fprintf(stderr,   "------------------------------\n");
  fflush(stderr);
}

int
main(int argc, char **argv)
{
  struct event_base *base;
  struct event *es2, *es3;
  struct evstomp_handle *h;

  talloc_enable_leak_report_full();

  base = event_base_new();
  h = evstomp_init(base, "atlantis.sysmgr.org", 61613);
  if (h == NULL) {
    fprintf(stderr, "Could not open stomp library.\n");
    abort();
  }
  es2 = evsignal_new(base, SIGUSR1, h_usr1, NULL);
  es3 = evsignal_new(base, SIGINT, sample3, NULL);
  evsignal_add(es2, NULL);
  evsignal_add(es3, NULL);

  event_base_dispatch(base);

  /* XXX evstomp_destroy(h);*/
  event_free(es2);
  event_free(es3);
  event_base_free(base);
  
  fprintf(stderr, "INFO: exiting.\n");
  exit(0);
}

