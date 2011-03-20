

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

struct header {
  char *name;
  char *value;
};

struct frame {
  char *type;
  struct header **headers;
  int headers_count;
  char *body;
};

struct evstomp_handle {
  struct event_base *base;
  regex_t re_parse_header;
  struct bufferevent *bev;
  int req_state;
  struct frame *incoming;
  char *sessionid;
  void (*cbfunc)(struct evstomp_handle *, enum evstomp_event_type, struct frame *, void *);
  void *cbfuncarg;
};

void
frame_set_header(struct frame *f, char* name, char* value)
{
  int pos;
  for (pos = 0; pos < f->headers_count; pos++) {
    if (strcmp(name, f->headers[pos]->name) == 0) {
      // deallocate old string, replace it with new one:
      talloc_free(f->headers[pos]->value);
      f->headers[pos]->value = talloc_strdup(f->headers[pos], value);
      return;
    }
  }
  // we couldn't find the header, and pos is one past the end of the
  //  array, so realloc...
  if (f->headers_count == 0) {
    f->headers = talloc_array(f, struct header*, ++f->headers_count);
  } else {
    f->headers = talloc_realloc(f, f->headers, struct header*,
        ++f->headers_count);
  }
  f->headers[pos] = talloc(f, struct header);
  f->headers[pos]->name = talloc_strdup(f->headers[pos], name);
  f->headers[pos]->value = talloc_strdup(f->headers[pos], value);
}

const char *
frame_get_header(struct frame *f, char* name)
{
  int pos;
  if (f->headers == NULL)
    return NULL;
  for (pos = 0; pos < f->headers_count; pos++) {
    if (strcmp(name, f->headers[pos]->name) == 0) {
      return f->headers[pos]->value;
    }
  }
  return NULL;
}

const char *
frame_get_body(struct frame *f) {
  return f->body;
}

void
process_frame(struct evstomp_handle *h, struct frame *f) {
  if (strcmp(f->type, "CONNECTED") == 0) {
    if (frame_get_header(f, "session") != NULL) {
      h->sessionid = talloc_strdup(h, frame_get_header(f, "session"));
    }
    if (h->cbfunc != NULL) {
      (*h->cbfunc)(h, CONNECTED, f, h->cbfuncarg);
    }
  } else if (strcmp(f->type, "MESSAGE") == 0) {
    if (h->cbfunc != NULL) {
      (*h->cbfunc)(h, MESSAGE, f, h->cbfuncarg);
    }
  } else if (strcmp(f->type, "ERROR") == 0) {
    if (h->cbfunc != NULL) {
      (*h->cbfunc)(h, ERROR, f, h->cbfuncarg);
    }
  }
}

void
evstomp_eventcb(struct bufferevent *bev, short events, void *arg)
{
  struct evstomp_handle *h = arg;
  if (events & BEV_EVENT_CONNECTED) {
    h->req_state = 1;
    if (h->incoming != NULL)
      talloc_free(h->incoming);
    h->incoming = talloc_zero(h, struct frame);
  } else if (events & BEV_EVENT_ERROR) {
    fprintf(stderr, "ERROR: error while connecting\n");
    exit(1);
  }
}

void
evstomp_readcb(struct bufferevent *bev, void *arg)
{
  struct evstomp_handle *h = arg;
  struct evbuffer *input = bufferevent_get_input(bev);
  char *lineout;
  int cont = 1;
  /* struct evbuffer_ptr eptr; */
  char *bufa;
  int bufapos;

  while (cont) {
    switch (h->req_state) {
      case 1: /* waiting for frame type line */
        lineout = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF);
        if (lineout == NULL) {
          cont = 0;
        } else if (strlen(lineout) == 0) {
          free(lineout);
        } else {
          talloc_free(h->incoming);
          h->incoming = talloc_zero(h, struct frame);
          h->incoming->type = talloc_strdup(h->incoming, lineout);
          free(lineout);
          h->req_state = 2;
        }
        break;
      case 2: /* read all header input lines */
        lineout = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF);
        if (lineout == NULL) {
          cont = 0;
        } else if (strlen(lineout) == 0) {
          h->req_state = 3;
        } else if (strstr(lineout, ":") != NULL) {
#define REGMATCH_LEN 5
          regmatch_t m[REGMATCH_LEN];
          char *a, *b;
          if (regexec(&h->re_parse_header, lineout,
              REGMATCH_LEN, m, 0) != 0) {
            fprintf(stderr, "DEBUG: header line did not match regex: %s\n",
                lineout);
          } else {
            a = talloc_strndup(h, lineout + m[1].rm_so,
                m[1].rm_eo - m[1].rm_so);
            b = talloc_strndup(h, lineout + m[2].rm_so,
                m[2].rm_eo - m[2].rm_so);
            frame_set_header(h->incoming, a, b);
            talloc_free(a);
            talloc_free(b);
	  }
#undef REGMATCH_LEN
        } else {
          fprintf(stderr, "INFO: MSG %s: non-HDR '%s'\n",
              h->incoming->type, lineout);
        }
        free(lineout);
        break;
      case 3: /* reading message body */
        /* ensure buffer is a contiguous block of memory
             so that we can search for the NUL */
        bufa = evbuffer_pullup(input, -1);
        for (bufapos = 0; bufapos < evbuffer_get_length(input); bufapos++) {
          if (bufa[bufapos] == 0) {
            h->incoming->body = talloc_size(h->incoming, bufapos + 1);
            talloc_set_name_const(h->incoming->body, "stomp msg body");
            evbuffer_remove(input, h->incoming->body, bufapos + 1);
            break;
          }
        }
        if (h->incoming->body != NULL) {
          process_frame(h, h->incoming); /* XXX */
          h->req_state = 1;
        } else {
          cont = 0;
        }
        break;
    }
  }
/*
  }*/
}

int
evstomp_subscribe(struct evstomp_handle *h, char *topic)
{
  char *tosend = talloc_asprintf(h, "SUBSCRIBE\ndestination: %s\nack: auto\n\n\0", topic);
  int rc = evbuffer_add(bufferevent_get_output(h->bev), tosend, strlen(tosend) + 1);
  talloc_free(tosend);
  if (rc == 0)
    return 0;
  else
    return -1;
  /* XXX should ask for a RECEIPT and then track this RECEIPT to see if successful */
}

void
evstomp_setcb(struct evstomp_handle *h,
    void (*cbfunc)(struct evstomp_handle *, enum evstomp_event_type,
    struct frame *, void *arg), void *arg)
{
  h->cbfuncarg = arg;
  h->cbfunc = cbfunc;
}

int
evstomp_handle_destructor(void *ptr)
{
  struct evstomp_handle *h = ptr;
  regfree(&h->re_parse_header);
  if (h->bev != NULL) {
    bufferevent_free(h->bev);
  }
  return 0;
}

struct evstomp_handle *
evstomp_init(struct event_base *base, char *hostname, int port)
{
  struct evstomp_handle *h;

  h = talloc_zero(ctx, struct evstomp_handle);
  if (h == NULL) {
    return NULL;
  }
  talloc_set_destructor(h, evstomp_handle_destructor);
  h->base = base;
  if (regcomp(&h->re_parse_header, "[ ]*([^:]+)[ ]*:[ ]*(.+)[ ]*",
      REG_EXTENDED) != 0) {
    talloc_free(h);
    return NULL;
  }

  h->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
  if (h->bev == NULL) {
    talloc_free(h);
    return NULL;
  }
#define TOSEND "CONNECT\n\n"
  evbuffer_add(bufferevent_get_output(h->bev), TOSEND, sizeof(TOSEND));
#undef  TOSEND
  bufferevent_setcb(h->bev, evstomp_readcb, NULL, evstomp_eventcb,
      h);
  if (bufferevent_socket_connect_hostname(h->bev, NULL, AF_INET, hostname,
      port) < 0) {
    talloc_free(h);
    return NULL;
  }
  bufferevent_enable(h->bev, EV_READ|EV_WRITE);

  return h;
}

