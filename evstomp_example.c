

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

#include "jmcstr.h"
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

void
print_msg_body(const char *body)
{
  regex_t re_msg, re_sbj;
  char **p, **s = jmcstr_split_lines(NULL, body);
  char *msg = NULL, *sbj = NULL, *dat = NULL;
  regcomp(&re_msg, "^Message: *(.*)$", REG_EXTENDED);
  regcomp(&re_sbj, "^Subject: *(.*)$", REG_EXTENDED);
  for (p = s; *p != NULL; p++) {
    regmatch_t m[2];
    if (regexec(&re_msg, *p, 2, m, 0) == 0) {
      msg = talloc_strndup(s, *p + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
    } else if (regexec(&re_sbj, *p, 2, m, 0) == 0) {
      sbj = talloc_strndup(s, *p + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
    }
  }
  dat = jmcstr_ftime_now(s, NULL);
  fprintf(stdout, "-------------------------------------------\n"
                  "Date:    %s\n"
                  "Subject: %s\n"
                  "Message: %s\n\n", dat, sbj, msg);
  talloc_free(s);
}

void
cbf(struct evstomp_handle *h, enum evstomp_event_type et, struct frame *f, void *arg) {
  switch (et) {
    case CONNECTED:
      fprintf(stderr, "CALLBACK: Connected, subscribing to the thing.\n");
      evstomp_subscribe(h, "/topic/notifications");
      break;
    case MESSAGE:
      /* fprintf(stderr, "CALLBACK: Message: %s ------------------\n"
              "%s\n-----------------------\n",
               frame_get_header(f, "destination"), frame_get_body(f)); */
      print_msg_body(frame_get_body(f));
      break;
    default:
      return;
  }
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
  evstomp_setcb(h, cbf, NULL);
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

