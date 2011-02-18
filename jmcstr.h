
#include <talloc.h>

#ifndef __JMCSTR_H__
#define __JMCSTR_H__

char **jmcstr_split_lines(TALLOC_CTX *ctx, const char *in);
char *jmcstr_ftime_now(TALLOC_CTX *ctx, const char *fmt);


#endif /* __JMCSTR_H__ */

