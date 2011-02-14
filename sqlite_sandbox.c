

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>

#include <talloc.h>
#include <sqlite3.h>

#define STUFF_END ((void*)0x7)

struct stuff {
  sqlite3 *sql;
  sqlite3_stmt *stmt;
};

struct stuff *x_init(char *dbname, char *query) {
  struct stuff *t = talloc(NULL, struct stuff);
  if (sqlite3_open(dbname, &t->sql) != SQLITE_OK) {
    printf("ERROR: While Opening: %s\n", sqlite3_errmsg(t->sql));
    sqlite3_close(t->sql);
    talloc_free(t);
    return NULL;
  }
  if (sqlite3_prepare(t->sql, query, strlen(query), &t->stmt, NULL)!= SQLITE_OK) {
    printf("ERROR: While Preparing: %s\n", sqlite3_errmsg(t->sql));
    sqlite3_close(t->sql);
    talloc_free(t);
    return NULL;
  }
  return t;
}

char **x_get_val_array(struct stuff *s) {
  int sz = 3;
  int pos = -1;
  char **out = talloc_array(s, char*, sz);

  if (sqlite3_reset(s->stmt) != SQLITE_OK) {
    printf("ERROR: While Resetting PS: %s\n", sqlite3_errmsg(s->sql));
    return NULL;
  }

  for (;;) {
    const unsigned char* n;
    int e = sqlite3_step(s->stmt);
    if (e == SQLITE_DONE)
      break;
    else if (e != SQLITE_ROW) {
      printf("ERROR: While stepping (%d): %s\n", e, sqlite3_errmsg(s->sql));
      /* XXX: Free stuff */
      return NULL;
    }
    n = sqlite3_column_text(s->stmt, 1);
    if (++pos >= sz) {
      /* extend array */
      sz += 3;
      out = talloc_realloc(s, out, char*, sz);
    }
    *(out + pos) = talloc_strdup(out, n);
  }

  /* place end-of-array marker */
  if (++pos >= sz) {
    sz += 1;
    out = talloc_realloc(s, out, char*, sz);
  }
  *(out + pos) = STUFF_END;

  return out;
}

void x_destroy_val_array(char **out) {
  talloc_free(out);
}

void x_free(struct stuff *s) {
  sqlite3_finalize(s->stmt);
  sqlite3_close(s->sql);
  talloc_free(s);
}

int do_diag = 0;
int do_quit = 0;
void hsig(int sig) {
  switch (sig) {
    case SIGUSR1:
      do_diag = 1;
      (void) signal(SIGUSR1, hsig);
      break;
    case SIGTERM:
    case SIGINT:
    case SIGUSR2:
      do_quit = 1;
      (void) signal(sig, hsig);
      break;
  }
}

int
main(int argc, char **argv) {
  struct stuff *stuff;
  char *q = "SELECT ID, NAME FROM AWESOME";

  (void) signal(SIGUSR1, hsig);
  (void) signal(SIGUSR2, hsig);
  (void) signal(SIGINT, hsig);
  (void) signal(SIGTERM, hsig);
  talloc_enable_leak_report_full();

  stuff = x_init("/tmp/awesome.sqlite3", q);
  if (stuff == NULL) { 
    printf("ERROR: could not init stuff\n");
    abort();
  }

  for (;;) {
    char **pos;
    char **vals = x_get_val_array(stuff);
    for (pos = vals; *pos != STUFF_END; pos++) {
      printf("%s, ", *pos);
    }
    x_destroy_val_array(vals);
    printf("\n-----\n");
    if (do_diag) {
      do_diag = 0;
      talloc_report_full(NULL, stderr);
      printf("=========\n");
    }
    if (do_quit) {
      break;
    }
    sleep(1);
  }

  x_free(stuff);
  fprintf(stderr, "======= end of run report: =====\n");
  talloc_report_full(NULL, stderr);
}


