#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/x86_full_empty.h"

#ifdef __APPLE__
#define MAP_ANONYMOUS MAP_ANON
#endif

/* XXX: No buffer size checking, etc. */

static void
next_line (FILE * in, char * rowbuf, size_t sz)
{
  char * c;
  do {
    memset (rowbuf, 0, sz*sizeof(*rowbuf));
    c = fgets (rowbuf, sz-1, in);
  } while (c && *c == '%');
  if (!c) {
    perror ("Confused.");
    abort ();
  }
}

int
load_metisish_graph (struct stinger * S, char * filename)
{
  FILE * fp = fopen (filename, "r");
  char rowbuf[16385];
  int64_t nv = 0, ne_ent = 0, ne = 0, fmt = 0, ncon = 0;
  int has_weight = 0;

  if (!fp)
  {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = '\0';
    perror (errmsg);
    exit (-1);
  }

  /* XXX: no I/O error checking... */

  next_line (fp, rowbuf, sizeof (rowbuf));

  sscanf (rowbuf, "%" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64,
	  &nv, &ne_ent, &fmt, &ncon);
  has_weight = fmt % 10;
  /* fprintf (stderr, "LINE: %s\n", rowbuf); */

  /* fprintf (stderr, "wtfwtf %ld %ld\n", (long)nv, (long)stinger_max_nv (S)); */

  for (int64_t i = 0; i < nv; ++i) {
    int64_t k, j, w = 1;
    size_t line_len;
    int pos = 0, cp, posold;
    next_line (fp, rowbuf, sizeof (rowbuf));
    line_len = strlen (rowbuf);
    /* fprintf (stderr, "LINE: %s\n", rowbuf); */
    /* fprintf (stderr, "%" PRId64  "...:", i); */
    for (int k2 = 0; k2 < ncon; ++k2) { /* Read and ignore. */
      sscanf (&rowbuf[pos], "%" SCNd64 "%n", &k, &cp);
      pos += cp;
    }
    do {
      posold = pos;
      j = -1;
      sscanf (&rowbuf[pos], "%" SCNd64 "%n", &j, &cp);
      if (j <= 0) break;
      --j;
      pos += cp;
      if (has_weight) {
	sscanf (&rowbuf[pos], "%" SCNd64 "%n", &w, &cp);
	pos += cp;
      }
      /* fprintf (stderr, " (%" PRId64 "; %" PRId64 ")", j+1, w); */

      /* Let duplicates be inserted here... */
      stinger_insert_edge (S, /*type*/ 0, i, j, w, /*time*/ 0);

    } while (pos < line_len && pos != posold);
  }

  fclose (fp);

  return 0;
}
