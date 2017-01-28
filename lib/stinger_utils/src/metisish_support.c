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
#include "stinger_core/formatting.h"
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

#define EBUFSZ 4192

static int64_t n_ebuf = 0;
static int64_t ebuf_i[EBUFSZ];
static int64_t ebuf_j[EBUFSZ];
static int64_t ebuf_w[EBUFSZ];

#if !defined(OMP)
#if defined(_OPENMP)
#define OMP(x_) _Pragma(x_)
#else
#define OMP(x_)
#endif
#endif

static void
flush_ebuf (struct stinger * S)
{
  if (n_ebuf > 0) {
    OMP("omp parallel") {
      char buf[65];
      OMP("omp for")
      for (int64_t k = 0; k < n_ebuf; ++k) {
        int64_t id_i, id_j;
        sprintf (buf, "%ld", (long)ebuf_i[k]);
        stinger_mapping_create (S, buf, strlen(buf), &id_i);
        sprintf (buf, "%ld", (long)ebuf_j[k]);
        stinger_mapping_create (S, buf, strlen(buf), &id_j);
        stinger_insert_edge (S, /*type*/ 0, id_i, id_j, ebuf_w[k],
                             /*time*/ 0);
      }
    }
  }
  n_ebuf = 0;
}

static inline void
push_ebuf (struct stinger * S, int64_t i, int64_t j, int64_t w)
{
  int64_t where = n_ebuf++;
  if (where >= EBUFSZ) {
    flush_ebuf (S);
    where = n_ebuf++;
  }
  ebuf_i[where] = i;
  ebuf_j[where] = j;
  ebuf_w[where] = w;
}

int
load_metisish_graph (struct stinger * S, char * filename)
{
  FILE * fp = fopen (filename, "r");
  char rowbuf[16385];
  int64_t nv = 0, ne_ent = 0, fmt = 0, ncon = 0;
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
      push_ebuf (S, i, j, w);

    } while (pos < line_len && pos != posold);
  }
  flush_ebuf (S);

  fclose (fp);

  return 0;
}
