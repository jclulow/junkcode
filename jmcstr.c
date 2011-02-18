
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <talloc.h>

#include "jmcstr.h"

/* XXX fix error handling to not suck */
#define NULL_ABORT(x) if ((x) == NULL) { printf("COULD NOT ALLOCATE MEMORY\n"); abort(); }

char **
jmcstr_split_lines(TALLOC_CTX *ctx, const char *in)
{
  int elems = 0;
  int totlen = 0;
  int inlen = strlen(in);
  char **out = talloc_array(ctx, char*, 1);
  NULL_ABORT(out)
  for (;;) {
    size_t sz = strcspn(in, "\r\n");
    out = talloc_realloc(ctx, out, char*, ++elems + 1);
    NULL_ABORT(out)
    out[elems - 1] = talloc_strndup(out, in, sz);
    NULL_ABORT(out[elems - 1])
    if (totlen + sz < inlen && in[sz] == '\r') sz += 1;
    if (totlen + sz < inlen && in[sz] == '\n') sz += 1;
    in += sz;
    totlen += sz;
    if (totlen >= inlen)
      break;
  }
  out[elems] = NULL;
  return out;
}

char *
jmcstr_ftime_now(TALLOC_CTX *ctx, const char *fmt)
{
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  char timebuf[80];
  char *out;
  strftime(timebuf, 80, fmt ? fmt : "%c", lt);
  out = talloc_strdup(ctx, timebuf);
  NULL_ABORT(out);
  return out;
}

