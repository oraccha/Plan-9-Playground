#include <u.h>
#include <libc.h>
#include "json.h"

enum {
  BUFSIZ = 64 * 1024,
};

void
main(int argc, char **argv)
{
  int f, n;
  char *buf;
  Json *jv;

  f = open(argv[1], OREAD);
  if(f < 0)
	sysfatal("open");

  buf = malloc(BUFSIZ);
  if(buf == nil)
    sysfatal("malloc");

  n = read(f, buf, BUFSIZ);
  print("read %d bytes.\n");

  jv = parsejson(buf);
  if(jv == nil)
    sysfatal("jsonparse");

  printjval(jv);

  exits(nil);
}
