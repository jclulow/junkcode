
#include <stdlib.h>
#include <stdio.h>

#include <talloc.h>

struct awesome {
  char *name;
  int rating;
};

int
main(int argc, char **argv) {
  int i = 0;
  struct awesome *a;

  talloc_enable_leak_report_full();

  a = talloc(NULL, struct awesome);
  talloc_set_name_const(a, "an awesome struct!");
  a->name = talloc_asprintf(a, "this is awesome struct #%d", ++i);


  talloc_free(a);

}


