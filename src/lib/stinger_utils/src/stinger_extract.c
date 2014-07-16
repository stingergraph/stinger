#include <stdio.h>
#include <assert.h>

#include "stinger_utils.h"
#include "timer.h"

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"

#if defined(_OPENMP)
#include <omp.h>
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

void
stinger_extract_bfs (/*const*/ struct stinger *S,
                     const int64_t nsrc, const int64_t * srclist_in,
                     const int64_t * label_in,
                     const int64_t max_nv_out,
                     const int64_t max_nlevels,
                     int64_t * nv_out,
                     int64_t * vlist_out /* size >=max_nv_out */,
                     int64_t * mark_out /* size nv, zeros */)
{
  const int64_t * restrict srclist = srclist_in;
  const int64_t * restrict label = label_in;
  int64_t * restrict vlist = vlist_out;
  int64_t * restrict mark = mark_out;
  const int64_t nlev = (max_nlevels > 0? max_nlevels : max_nv_out);

  const int64_t label_to_match = (label && nsrc? label[srclist[0]] : -1);
  /* XXX: Arbitrary: pick first source label, but include all sources. */

  int64_t nv, f_start, new_nv, lev;

  if (!nsrc) {
    *nv_out = 0;
    return;
  }

  if (nsrc >= max_nv_out) {
    /* Just in case. */
    for (int64_t k = 0; k < max_nv_out; ++k) {
      const int64_t v = srclist[k];
      vlist[k] = v;
      mark[v] = k+1;
    }
    *nv_out = max_nv_out;
    return;
  }

  for (int64_t k = 0; k < nsrc; ++k) {
      const int64_t v = srclist[k];
      vlist[k] = v;
      mark[v] = k+1;
  }

  nv = new_nv = nsrc;
  f_start = 0;
  lev = 0;
  while (nv < max_nv_out && lev < nlev) {
    for (int64_t k = f_start; k < nv; ++k) {
      const int64_t v = vlist[k];
      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, v) {
        const int64_t w = STINGER_EDGE_DEST;
        if (label && label_to_match != label[w]) continue;
        if (!mark[w] && new_nv < max_nv_out) {
          vlist[new_nv] = w;
          mark[w] = ++new_nv;
        }
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }
    f_start = nv;
    nv = new_nv;
    ++lev;
  }
  *nv_out = nv;
}

void
stinger_extract_mod (/*const*/ struct stinger *S,
                     const int64_t nsrc, const int64_t * srclist_in,
                     const int64_t * label_in,
                     const int64_t max_nv_out,
                     int64_t * nv_out,
                     int64_t * vlist_out /* size >=max_nv_out */,
                     int64_t * mark_out /* size nv, zeros */)
{
  const int64_t NV = S->max_nv;
  const int64_t * restrict srclist = srclist_in;
  const int64_t * restrict label = label_in;
  int64_t * restrict vlist = vlist_out;
  int64_t * restrict mark = mark_out;

  const int64_t label_to_match = (label && nsrc? label[srclist[0]] : -1);
  const int64_t totvol = S->cur_ne / 2; /* assuming undirected */

  int64_t nset, frontier_end;
  double setvol = 0.0;
  /* set holds the current set and the frontier, more than max_nv_out */
  int64_t * restrict set;
  int64_t * restrict Wvol; /* dim NV x 2 */

  if (!nsrc) {
    *nv_out = 0;
    return;
  }

  if (nsrc >= max_nv_out) {
    /* Just in case. */
    for (int64_t k = 0; k < max_nv_out; ++k) {
      const int64_t v = srclist[k];
      vlist[k] = v;
      mark[v] = k+1;
    }
    *nv_out = max_nv_out;
    return;
  }

  set = xmalloc (NV * sizeof (*set));
  Wvol = xmalloc (2 * NV * sizeof (*Wvol));

  /* Initialize the set with the seeds and their neighbors */
  nset = nsrc;
  frontier_end = nsrc;
  for (int64_t k = 0; k < nsrc; ++k) {
    const int64_t v = srclist[k];
    set[k] = v;
    mark[v] = k+1; /* Must be set before expanding frontier... */
    /* Eh, just in case... */
    Wvol[2*k] = 0;
    Wvol[1+2*k] = 0;
  }
  for (int64_t k = 0; k < nset; ++k) {
    const int64_t u = set[k];
    int64_t uvol = 0;
    int64_t vW = 0, vvol = 0;
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, u) {
      const int64_t v = STINGER_EDGE_DEST;
      int64_t where;

      if (label && label_to_match != label[v]) continue;

      ++uvol;
      where = mark[v]-1;
      if (where >= 0) {
        if (where > nset) Wvol[2*where] += 1; /* No need to update aborbed ones... */
        continue;
      }

      /* Append... */
      where = frontier_end++;
      assert (where >= 0);
      assert (where < NV);
      /* Annoying bit: will walk edges twice. */
      set[where] = v;
      mark[v] = where+1;
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
        const int64_t z = STINGER_EDGE_DEST;
        if (label && label_to_match != label[z]) continue;
        ++vvol;
        if (mark[z] > 0) ++vW;
      } STINGER_FORALL_EDGES_OF_VTX_END();
      Wvol[2*where] = vW;
      Wvol[1+2*where] = vvol;
    } STINGER_FORALL_EDGES_OF_VTX_END();
    setvol += uvol;
  }

  /*
    While the output list isn't full, find the best in the frontier,
    move it into the set (first of frontier, expand set), and
    re-expand the frontier.
  */
  while (nset < max_nv_out) {
    int64_t best_loc = nset;
    const double setvol_scale = setvol / (double)totvol;
    double best_score = Wvol[2*best_loc] - Wvol[1+2*best_loc] * setvol_scale;
    for (int64_t k = nset+1; k < frontier_end; ++k) {
      double score = Wvol[2*k] - Wvol[1+2*k] * setvol_scale;
      if (score > best_score || (score == best_score && (Wvol[2*k] > Wvol[2*best_loc]))) {
        best_loc = k;
        best_score = score;
      }
    }

    if (best_score < 0) break; /* Nothing improves modularity, stop. */

    const int64_t u = set[best_loc];

    setvol += Wvol[1+2*best_loc];

    if (best_loc != nset) {
      int64_t t = set[nset];
      set[best_loc] = t;
      set[nset] = u;
      assert(mark[u] == best_loc+1);
      assert(mark[t] == nset+1);
      mark[u] = nset+1;
      mark[t] = best_loc+1;
      Wvol[2*best_loc] = Wvol[2*nset];
      Wvol[1+2*best_loc] = Wvol[1+2*nset];
      /* No longer need Wvol for new set member */
    }
    ++nset;

    /* expand u */
    int64_t vW = 0, vvol = 0;
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, u) {
      const int64_t v = STINGER_EDGE_DEST;
      int64_t where;

      if (label && label_to_match != label[v]) continue;

      where = mark[v]-1;
      if (where >= 0) {
        if (where > nset) Wvol[2*where] += 1; /* No need to update aborbed ones... */
        continue;
      }

      /* Append... */
      where = frontier_end++;
      assert (where >= 0);
      assert (where < NV);
      /* Annoying bit: will walk edges twice. */
      set[where] = v;
      mark[v] = where+1;
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
        const int64_t z = STINGER_EDGE_DEST;
        if (label && label_to_match != label[z]) continue;
        ++vvol;
        if (mark[z] > 0) ++vW;
      } STINGER_FORALL_EDGES_OF_VTX_END();
      Wvol[2*where] = vW;
      Wvol[1+2*where] = vvol;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  /* Copy set to the output, clear the frontier's locations */
  assert(nset <= max_nv_out);
  *nv_out = nset;
  for (int64_t k = 0; k < nset; ++k) {
    vlist[k] = set[k];
    assert(mark[set[k]] == k+1);
  }
  for (int64_t k = nset; k < frontier_end; ++k)
    mark[set[k]] = 0;

  free (Wvol);
  free (set);
}

