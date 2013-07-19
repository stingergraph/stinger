#define _GNU_SOURCE

#include <stdlib.h>
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "compat/luc.h"
#include "graph_el_support.h"
#include "stinger_utils.h"

int
load_graph_el (struct stinger * S, const char * filename)
{
  int64_t ne, ne2;
  struct el g;

  read_graph_el (filename, &g);

  DECL_EL(g);

  ne = 2 * g.ne;
  for (int64_t i = 0; i < g.nv; i++)
    if (D(g,i) > 0)
      ne++;

  /* allocate temporary arrays */
  int64_t * sV = xmalloc (ne * sizeof(int64_t));
  int64_t * eV = xmalloc (ne * sizeof(int64_t));
  int64_t * wgt = xmalloc (ne * sizeof(int64_t));

  ne2 = 0;

  MTA("mta assert nodep")
  for (int64_t i = 0; i < g.nv; i++)
  {
    if (D(g,i) > 0) {
      int64_t k = ne2++;
      sV[k] = i;
      eV[k] = i;
      wgt[k] = D(g,i);
    }
  }

  MTA("mta assert nodep")
  for (int64_t i = 0; i < g.ne; i++)
  {
    int64_t k1 = ne2++;
    int64_t k2 = ne2++;
    sV[k1] = I(g,i);
    eV[k1] = J(g,i);
    wgt[k1] = W(g,i);
    sV[k2] = J(g,i);
    eV[k2] = I(g,i);
    wgt[k2] = W(g,i);
  }

  free_graph_el (&g);

  // TODO: compute graph

  /* free temporary arrays */
  free (wgt);
  free (eV);
  free (sV);

  return 0;
}

static void
free_el (struct el * g)
{
  if (!g || !g->mem) return;
  munmap (g->mem, g->memsz);
  xelemset (g, 0, sizeof (*g));
}


void
read_graph_el (const char * fname, struct el * g)
{
  const uint64_t endian_check = 0x1234ABCDul;
  size_t sz;
  int64_t *mem;

  luc_io_init ();
  luc_stat (fname, &sz);

  if (sz % sizeof (*mem)) {
    perror ("Graph file size is not a multiple of sizeof (int64_t)");
    exit (-1);
  }

  mem = xmmap (NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

  luc_snapin (fname, mem, sz);

  if (endian_check != *mem)
    bs64_n (sz / sizeof (*mem), mem);

  g->nv = mem[1];
  g->ne = mem[2];

  g->mem = mem;
  g->memsz = sz;

  g->d = &mem[3];
  g->el = &mem[3+g->nv];
  g->free = free_el;
}

  void
free_graph_el (struct el * g)
{
  if (g && g->mem && g->free) g->free (g);
}

