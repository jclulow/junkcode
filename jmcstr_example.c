
#include <stdlib.h>
#include <stdio.h>
#include "jmcstr.h"

char *str_to_split = "Some kind of string with\r\nCRLF line stuff\nin it\n\nyeeeah\rsure.";

int
main(int argc, char** argv)
{
  char **pos, **a = jmcstr_split_lines(NULL, str_to_split);
  for (pos = a; *pos != NULL; pos++) {
    printf("|%s|\n", *pos);
  }
}

