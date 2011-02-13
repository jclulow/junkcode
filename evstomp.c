

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

struct connstate {
  struct bufferevent *bev;
  int req_state;
  struct frame *incoming;
  char *sessionid;
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

void
process_frame(struct connstate *cs, struct frame *f) {
  struct evbuffer *output = bufferevent_get_output(cs->bev);
  fprintf(stderr, "DEBUG: processing frame: '%s'\n", f->type);
  if (strcmp(f->type, "CONNECTED") == 0) {
#define TOSEND "SUBSCRIBE\nreceipt: johnfrost\ndestination: /topic/notifications\nack: auto\n\n"
    evbuffer_add(output, TOSEND, sizeof(TOSEND));
#undef  TOSEND
    if (frame_get_header(f, "session") != NULL) {
      fprintf(stderr, "INFO: session id == %s\n", frame_get_header(f, "session"));
      cs->sessionid = talloc_strdup(cs, frame_get_header(f, "session"));
    } else {
      fprintf(stderr, "INFO: no session id ??!\n");
    }
  } else if (strcmp(f->type, "MESSAGE") == 0) {
    fprintf(stderr, "\n===MESSAGE==================================\n");
    fprintf(stderr, "  dst: %s\n", frame_get_header(f, "destination"));
    fprintf(stderr, "  len: %s\n", frame_get_header(f, "content-length"));
    fprintf(stderr, "   id: %s\n", frame_get_header(f, "message-id"));
    fprintf(stderr,   "--------------------------------------------\n");
    fprintf(stderr, "%s", f->body);
    fprintf(stderr, "\n^^=MESSAGE================================^^\n");
    fflush(stderr);
  } else if (strcmp(f->type, "ERROR") == 0) {
    fprintf(stderr, "\n===ERROR====================================\n");
    fprintf(stderr, "  msg: %s\n", frame_get_header(f, "message"));
    fprintf(stderr,   "--------------------------------------------\n");
    fprintf(stderr, "%s", f->body);
    fprintf(stderr, "\n^^=ERROR==================================^^\n");
    fflush(stderr);
  }
}

void
evstomp_eventcb(struct bufferevent *bev, short events, void *arg)
{
  struct connstate* cs = arg;
  if (events & BEV_EVENT_CONNECTED) {
    fprintf(stderr, "INFO: connected\n");
    cs->req_state = 1;
    cs->incoming = talloc_zero(cs, struct frame);
  } else if (events & BEV_EVENT_ERROR) {
    fprintf(stderr, "ERROR: error while connecting\n");
    exit(1);
  }
}

void
evstomp_readcb(struct bufferevent *bev, void *arg)
{
  struct connstate *cs = arg;
  int n;
  struct evbuffer *input = bufferevent_get_input(bev);
  char *lineout;
  int cont = 1;
  /* struct evbuffer_ptr eptr; */
  char *bufa;
  int bufapos;

  /*while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) { */
  while (cont) {
    // XXX fprintf(stderr, "DEBUG: top of while\n");
    switch (cs->req_state) {
      case 1:
        lineout = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF);
        if (lineout == NULL) {
          cont = 0;
        } else if (strlen(lineout) == 0) {
          // XXX fprintf(stderr, "DEBUG: ate blank line at the end of a message...\n");
          free(lineout);
        } else {
          // XXX fprintf(stderr, "INFO: server message type '%s'\n", lineout);
          talloc_free(cs->incoming);
          cs->incoming = talloc_zero(cs, struct frame);
          cs->incoming->type = talloc_strdup(cs->incoming, lineout);
          free(lineout);
          cs->req_state = 2;
        }
        break;
      case 2:
        lineout = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF);
        if (lineout == NULL) {
          cont = 0;
        } else if (strlen(lineout) == 0) {
          fprintf(stderr, "INFO: blank line received, end of %s header\n", 
              cs->incoming->type);
          cs->req_state = 3;
        } else if (strstr(lineout, ":") != NULL) {
          regex_t re;
          regmatch_t m[5];
          char *a, *b;
          fprintf(stderr, "INFO: regcomp() == %d\n", regcomp(&re, "[ ]*([^:]+)[ ]*:[ ]*(.+)[ ]*", REG_EXTENDED));
          fprintf(stderr, "INFO: MSG %s: HDR '%s'\n", 
              cs->incoming->type, lineout);
          fprintf(stderr, "INFO: regexec() == %d\n", regexec(&re, lineout, 5, m, 0));
          a = talloc_strndup(cs, lineout + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
          b = talloc_strndup(cs, lineout + m[2].rm_so, m[2].rm_eo - m[2].rm_so);
          fprintf(stderr, "INFO: m[0] = '%s'; m[1] = '%s'\n", a, b);
          frame_set_header(cs->incoming, a, b);
          talloc_free(a);
          talloc_free(b);
          regfree(&re);
        } else {
          fprintf(stderr, "INFO: MSG %s: non-HDR '%s'\n", 
              cs->incoming->type, lineout);
        }
        free(lineout);
        break;
      case 3:
        /* ensure buffer is a contiguous block of memory
             so that we can search for the NUL */
        bufa = evbuffer_pullup(input, -1);
        for (bufapos = 0; bufapos < evbuffer_get_length(input); bufapos++) {
          if (bufa[bufapos] == 0) {
            cs->incoming->body = talloc_size(cs->incoming, bufapos + 1);
            talloc_set_name_const(cs->incoming->body, "stomp msg body");
            evbuffer_remove(input, cs->incoming->body, bufapos + 1);
            fprintf(stderr, "INFO: NUL FOUND IN BODY (pos %d)!\n", bufapos);
            break;
          }
        }
        if (cs->incoming->body != NULL) {
          fprintf(stderr, "BODY: %s\n", cs->incoming->body);
          process_frame(cs, cs->incoming); /* XXX */
          cs->req_state = 1;
        } else {
          fprintf(stderr, "INFO: NUL not found in body yet\n");
          cont = 0;
        }
        break;
    }
  }
  fprintf(stderr, "DEBUG: end of readcb\n");
/*
  }*/
}

struct evstomp_handle {
  struct event_base *base;
  struct connstate *conn;
};

struct evstomp_handle *
evstomp_init(struct event_base *base, char *hostname, int port)
{
  struct event *es1, *es2, *es3;
  struct sockaddr* sa;
  struct connstate* cs;
  struct evstomp_handle *h;

  h = talloc(NULL, struct evstomp_handle);
  if (h == NULL) {
    return NULL;
  }
  h->base = base;

  h->conn = talloc(h, struct connstate);
  if (h->conn == NULL) {
    return NULL;
  }
  bzero(h->conn, sizeof(*h->conn));

  h->conn->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
  if (h->conn->bev == NULL) {
    talloc_free(h->conn);
    return NULL;
  }
#define TOSEND "CONNECT\n\n"
  evbuffer_add(bufferevent_get_output(h->conn->bev), TOSEND, sizeof(TOSEND));
#undef  TOSEND
  bufferevent_setcb(h->conn->bev, evstomp_readcb, NULL, evstomp_eventcb, 
      h->conn);
  if (bufferevent_socket_connect_hostname(h->conn->bev, NULL, AF_INET, hostname, 
      port) < 0) {
    bufferevent_free(h->conn->bev);
    talloc_free(h);
    return NULL;
  }
  bufferevent_enable(h->conn->bev, EV_READ|EV_WRITE);

  return h;
}

