

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <time.h>

#include <talloc.h>
/* #include <sqlite3.h> */
#include <event2/event.h>

int xx = 0;
int sigintcount = 0;

void
sample3(evutil_socket_t fd, short event, void *arg)
{
  fprintf(stderr, "SIGINT NUMBER %d\n", ++sigintcount);
  if (sigintcount >= 3) {
    exit(0);
  }
}

void
sample2(evutil_socket_t fd, short event, void *arg)
{
  xx++;
}

void
sample1(evutil_socket_t fd, short event, void *arg)
{
  fprintf(stderr, "EVENT: (fd %d) %hd - %s (%d)\n", fd, event, (char*)arg, xx);
  fflush(stderr);
}

int
main(int argc, char **argv)
{
  struct event_base *base;
  struct event *es1, *es2, *es3;
  struct timeval tv, tv2;

  tv.tv_sec = 0;
  tv.tv_usec = 500*1000;
  tv2.tv_sec = 0;
  tv2.tv_usec = 11*1000;

  base = event_base_new();
  
  //es1 = evtimer_new(base, sample1, "sample timer");
  es1 = event_new(base, -1, EV_PERSIST, sample1, "sample timer");
  es2 = event_new(base, -1, EV_PERSIST, sample2, NULL);
  es3 = evsignal_new(base, SIGINT, sample3, NULL);
  //evtimer_set(es1, sample1, "sample timer");
  //es1->ev_flags |= EV_PERSIST;
  evtimer_add(es1, &tv);
  evtimer_add(es2, &tv2);
  evsignal_add(es3, NULL);

  event_base_dispatch(base);

  event_free(es1);
  event_base_free(base);
  
  fprintf(stderr, "INFO: exiting.\n");
  exit(0);

}

