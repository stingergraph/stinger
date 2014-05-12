/* -*- mode: C; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "compat.h"
#include "sorts.h"
#include "graph-el.h"
#include "community.h"

#include "xmt-luc.h"

#include "bsutil.h"

#include "community-update.h"

static void update_el (struct el * el,
                       int64_t * cmap,
                       int64_t * csize,
                       const struct stinger * S,
                       const int64_t nvlist, const int64_t * restrict vlist,
                       int64_t * mark,
                       int64_t ** ws, size_t *wslen);

#define NEEDED_WSLEN_CONTRACT(nv_, ne_) (3*(nv_)+1 + 2*(ne_))
#define NEEDED_WSLEN_UPDATE(nv_, ne_) NEEDED_WSLEN_CONTRACT(nv_, ne_)
#define NEEDED_WSLEN_nv_ne_(nv_, ne_) ((ne_) < (nv_)? (nv_) : (ne_))
#define NEEDED_WSLEN_COMM(nv_, ne_) (6*(nv_) + 1 + 3*NEEDED_WSLEN_nv_ne_(nv_,ne_))

static inline void
realloc_ws (int64_t **ws, size_t *wslen, int64_t nv, int64_t ne)
{
  size_t needed_comm = NEEDED_WSLEN_COMM (nv, ne);
  size_t needed_update = NEEDED_WSLEN_UPDATE (nv, ne);
  size_t needed = (needed_comm > needed_update? needed_comm : needed_update);
  if (*wslen < needed) { /* XXX: this causes too many || 2*needed > *wslen) { */
    /* fprintf (stderr, "realloc %ld => %ld\n", (long)*wslen, (long)needed); */
    *ws = xrealloc (*ws, needed * sizeof (**ws));
    *wslen = needed;
  }
}

static void
init_community_state (struct community_state * cstate,
                     const int64_t graph_nv, const int64_t ne_est)
{
  memset (cstate, 0, sizeof (*cstate));
  cstate->graph_nv = graph_nv;
  cstate->cmap = xmalloc (graph_nv * sizeof (*cstate->cmap));
  cstate->csize = xmalloc (graph_nv * sizeof (*cstate->csize));
  cstate->mark = xmalloc (graph_nv * sizeof (*cstate->mark));
  cstate->vlist = xmalloc (graph_nv * sizeof (*cstate->vlist));
  cstate->nvlist = 0;
#if defined(_OPENMP)
  cstate->lockspace = xmalloc (graph_nv * sizeof (*cstate->lockspace));
#endif
  OMP("omp parallel") {
    OMP("omp for nowait") MTA("mta assert nodep")
      for (int64_t k = 0; k < graph_nv; ++k) {
        cstate->cmap[k] = k;
        cstate->csize[k] = 1;
        cstate->mark[k] = -1;
      }
#if defined(_OPENMP)
    OMP("omp parallel")
      for (intvtx_t i = 0; i < graph_nv; ++i) {
        omp_init_lock (&cstate->lockspace[i]);
    }
#endif
  }
  memset (&cstate->hist, 0, sizeof (cstate->hist));
  cstate->ws = NULL;
  cstate->wslen = 0;
  realloc_ws (&cstate->ws, &cstate->wslen, graph_nv, ne_est);

  cstate->alg_score = ALG_CNM;
  cstate->alg_match = ALG_GREEDY_MAXIMAL;
  if (getenv ("COMM_LIMIT")) {
    int64_t t = strtol (getenv ("COMM_LIMIT"), NULL, 10);
    if (t > 0) cstate->comm_limit = t;
    else fprintf (stderr, "Invalid COMM_LIMIT: %s\n", getenv ("COMM_LIMIT"));
  } else
    cstate->comm_limit = INT64_MAX;
}

static void
init_community_state_from_el (struct community_state * cstate,
                             const int64_t graph_nv, struct el * g)
{
  init_community_state (cstate, graph_nv, (g->ne < 2*graph_nv? g->ne : 2*graph_nv));

  cstate->cg = *g;
  memset (g, 0, sizeof (*g)); /* Own. */
}

void
init_empty_community_state (struct community_state * cstate,
                           const int64_t graph_nv, const int64_t ne_est)
{
  init_community_state (cstate, graph_nv, ne_est);

  cstate->cg = alloc_graph (graph_nv, ne_est);
  intvtx_t * restrict d = cstate->cg.d;
  OMP("omp parallel for")
    for (int64_t k = 0; k < graph_nv; ++k)
      d[k] = 0;
}

void
finalize_community_state (struct community_state * cstate)
{
  free_graph (cstate->cg);
#if defined(_OPENMP)
  OMP("omp parallel for")
    for (intvtx_t i = 0; i < cstate->graph_nv; ++i)
      omp_destroy_lock (&cstate->lockspace[i]);
  free (cstate->lockspace);
#endif
  free (cstate->vlist);
  free (cstate->mark);
  free (cstate->csize);
  free (cstate->cmap);
  free (cstate->ws);
  memset (cstate, 0, sizeof (*cstate));
}

int
cstate_check (struct community_state *cstate)
{
  const int64_t graph_nv = cstate->graph_nv;
  const int64_t nv = cstate->cg.nv;
  const int64_t ne = cstate->cg.ne;
  int64_t * restrict csize = cstate->csize;
  const int64_t * restrict cmap = cstate->cmap;
  const struct el el = cstate->cg;
  CDECL(el);
  OMP("omp parallel") {
  OMP("omp for nowait") MTA("mta assert parallel")
    for (int64_t k = 0; k < ne; ++k) {
      //fprintf (stderr, "%ld:  %ld %ld ; %ld\n", (long)k, (long)I(el,k), (long)J(el,k), (long)W(el,k));
      assert (I(el, k) < nv);
      assert (J(el, k) < nv);
      assert (I(el, k) >= 0);
      assert (J(el, k) >= 0);
    }
  OMP("omp for") MTA("mta assert parallel")
    for (int64_t k = 0; k < graph_nv; ++k) {
      assert (csize[k] > 0);
    }
  }
  return 1;
}

static int64_t
cstate_forcibly_update_csize (struct community_state *cstate)
{
  const int64_t graph_nv = cstate->graph_nv;
  const int64_t nv = cstate->cg.nv;
  int64_t * restrict csize = cstate->csize;
  const int64_t * restrict cmap = cstate->cmap;
  intvtx_t mxsz = 0;
  OMP("omp parallel") {
    intvtx_t tmxsz = 0;
    OMP("omp for")
      for (intvtx_t k = 0; k < nv; ++k)
        csize[k] = 0;
    OMP("omp for")
      for (intvtx_t k = 0; k < graph_nv; ++k) {
        assert(cmap[k] < nv);
        OMP("omp atomic") ++csize[cmap[k]];
      }
    OMP("omp for")
      for (intvtx_t k = 0; k < nv; ++k) {
        assert (csize[k] > 0);
        if (csize[k] > tmxsz) tmxsz = csize[k];
      }
    OMP("omp critical")
      if (tmxsz > mxsz) mxsz = tmxsz;
  }
  cstate->max_csize = mxsz;
  return mxsz;
}

double
init_and_compute_community_state (struct community_state * cstate, struct el * g)
{
  init_community_state_from_el (cstate, g->nv, g);
  realloc_ws (&cstate->ws, &cstate->wslen, cstate->cg.nv, cstate->cg.ne);
  tic ();
  community (cstate->cmap, &cstate->cg,
    cstate->alg_score, cstate->alg_match, 0, 0,
    cstate->comm_limit,
    1,
    -1, -1, -1, 0, 1.1, 1, &cstate->hist, NULL, 0,
    cstate->csize,
    cstate->ws, cstate->wslen,
    cstate->lockspace);
  shrinkwrap_graph (&cstate->cg);
  return toc ();
}

double
init_and_read_community_state (struct community_state * cstate, int64_t graph_nv,
                               const char *cg_name, const char *cmap_name)
{
  int needs_bs;
  struct el el;
  tic ();
  needs_bs = el_snarf_graph (cg_name, &el);
  init_community_state_from_el (cstate, graph_nv, &el);
  xmt_luc_snapin (cmap_name, cstate->cmap, cstate->graph_nv * sizeof (*cstate->cmap));
  if (needs_bs)
    comm_bs64_n (graph_nv, cstate->cmap);
  cstate_forcibly_update_csize (cstate);
  return toc ();
}

void
cstate_dump_cmap (struct community_state * cstate, long which, long num)
{
  char fname[257];
  FILE *f = NULL;
  sprintf (fname, "cmap_%08ld_%08ld.txt", which, num);
  f = fopen (fname, "w");
  for (int64_t k = 0; k < cstate->graph_nv; ++k)
    fprintf (f, "%ld\n", (long)cstate->cmap[k]);
  fclose (f);
}

double
cstate_update (struct community_state * cstate, const struct stinger * S)
{
  tic ();

  update_el (&cstate->cg, cstate->cmap, cstate->csize,
             S, cstate->nvlist, cstate->vlist, cstate->mark,
             &cstate->ws, &cstate->wslen);

#if !defined(NDEBUG)
  assert (cstate_check (cstate));
#endif

  realloc_ws (&cstate->ws, &cstate->wslen, cstate->cg.nv, cstate->cg.ne);

  cstate->nstep =
    update_community (cstate->cmap, cstate->graph_nv, cstate->csize,
                      &cstate->max_csize,
                      &cstate->n_nonsingletons,
                      &cstate->cg,
                      cstate->alg_score, cstate->alg_match, 0, 0,
                      cstate->comm_limit, 1,
                      -1, -1, -1, 0, 1.1, 0, &cstate->hist,
                      cstate->ws, cstate->wslen,
                      cstate->lockspace);

  if (cstate->nstep > 0) {
    cstate->modularity = eval_modularity_cgraph (cstate->cg, cstate->ws);
  }

  if (cstate->cg.ne_orig > 2*cstate->cg.ne)
    shrinkwrap_graph (&cstate->cg);

#if !defined(NDEBUG)
  assert (cstate_check (cstate));
#endif
  return toc ();
}

MTA("mta inline")
MTA("mta expect parallel context")
void
qflush (struct insqueue * restrict q,
        struct el * restrict g)
{
  PDECL(g);
#if !defined(NO_QUEUE)
  if (q->n != 0) {
    int64_t where;
#if !defined(NO_SORTUNIQ_INSQUEUE) && !defined(__MTA__)
    int64_t kdst, idst, jdst, wdst;
    shellsort_ikii (q->q, q->n); /* Ideallly, quick pre-collapse in L1 */
    kdst = 0;
    idst = q->q[0];
    jdst = q->q[1];
    wdst = q->q[2];
    for (int k = 1; k < q->n; ++k) {
      if (q->q[3*k] == idst && q->q[1+3*k] == jdst)
        wdst += q->q[2+3*k];
      else {
        q->q[2+3*kdst] = wdst;
        ++kdst;
        idst = q->q[3*k];
        jdst = q->q[1+3*k];
        wdst = q->q[2+3*k];
        if (k != kdst) {
          q->q[3*kdst] = idst;
          q->q[1+3*kdst] = jdst;
        }
      }
    }
    if (kdst) q->q[2+3*kdst] = wdst;
    ++kdst;
    where = int64_fetch_add (&g->ne, kdst);
    assert (g->ne <= g->ne_orig);
    for (int64_t k = 0; k < kdst; ++k) {
      I(g, where+k) = q->q[3*k];
      J(g, where+k) = q->q[1+3*k];
      W(g, where+k) = q->q[2+3*k];
    }
#else /* on XMT */
    /* Without locality, no point in pre-collapsing. */
    where = int64_fetch_add (&g->ne, q->n);
    assert (g->ne <= g->ne_orig);
    for (int64_t k = 0; k < q->n; ++k) {
      I(g, where+k) = q->q[3*k];
      J(g, where+k) = q->q[1+3*k];
      W(g, where+k) = q->q[2+3*k];
    }
#endif
    q->n = 0;
  }
#endif /* NO_QUEUE */
}

MTA("mta expect parallel context") MTA("mta inline")
void
enqueue (struct insqueue * restrict q,
         intvtx_t i, intvtx_t j, intvtx_t w,
         struct el * restrict g)
{
#if !defined(NO_QUEUE)
  int where = q->n;
  if (where > 0) {
    const int k = where-1;
    /* check if already there, not uncommon in sorted edge lists */
    if (q->q[3*k] == i && q->q[1+3*k] == j) {
      q->q[2+3*k] += w;
      return;
    }
  }
  if (where < INSQUEUE_SIZE) {
    ++q->n;
    q->q[3*where] = i;
    q->q[1+3*where] = j;
    q->q[2+3*where] = w;
    return;
  }

  /* ugh. */
  qflush (q, g);
  q->n = 1;
  q->q[0] = i;
  q->q[1] = j;
  q->q[2] = w;
#else
  PDECL(g);
  int64_t where = int64_fetch_add (&g->ne, 1);
  assert (g->ne <= g->ne_orig);
  I(g, where) = i;
  J(g, where) = j;
  W(g, where) = w;
#endif
}

MTA("mta expect parallel context") MTA("mta inline")
static inline void
extract_vertex (const int64_t v, const int64_t cv, const struct stinger * restrict S,
                int64_t * restrict mark,
                int64_t * restrict ncomm, int64_t * restrict csize)
{
  int64_t old_csize;

  if (csize[cv] > 1) {
    old_csize = int64_fetch_add (&csize[cv], -1);
    if (old_csize > 1) {
      /* Place vertex in a new community. */
      int64_t new_c;
      new_c = int64_fetch_add (ncomm, 1);
      mark[v] = new_c;
      csize[new_c] = 1;
    } else {
      /* if (old_csize != 1) fprintf (stderr, "augh %ld %ld\n", (long)old_csize, (long)csize[cv]); */
      assert (old_csize == 1);
      /* Last one out stays in the community. */
      OMP("omp atomic") ++csize[cv];
      mark[v] = -1;
    }
  } else {
    assert (csize[cv] == 1);
    mark[v] = -1;
  }
}

static void
extract_vertices (const int64_t nvlist, const int64_t * restrict vlist, const int64_t * restrict cmap,
                  const struct stinger * restrict S,
                  int64_t * restrict mark,
                  int64_t * restrict ncomm, int64_t * restrict csize)
{
  OMP("omp parallel for schedule(guided)") MTA("mta interleave schedule")
    for (int64_t k = 0; k < nvlist; ++k) {
      assert (vlist[k] >= 0);
      extract_vertex (vlist[k], cmap[vlist[k]], S, mark, ncomm, csize);
    }
}

/*
two options:
  1) scan whole list, insert only one side
       greater read traffic, more scattered atomic ops
  2) scan list, only walk new comm vtcs, insert all
       less BW, more dense ops, mitigate by queuing

  v: if mark[v] >= 0, is in new comm
       walk neighbors w
       remove old edges:
         if comm[v] != comm[w]
           insert edge (comm[v], comm[w], -weight)
         else
           subtract weight from d[comm[v]]
       insert new edges:
         if mark[w] < 0, old comm
           insert edge (mark[v], comm[w], weight)
         if else new comm
           if mark[w] > mark[v], insert (mark[v], mark[w], weight)
*/

static void
extract_edges (const int64_t nvlist, const int64_t * restrict vlist, const int64_t * restrict cmap,
               const struct stinger * restrict S,
               const int64_t * restrict mark,
               struct el * restrict g,
               const int64_t new_nv)
{
  PDECL(g);
  int64_t n_new_edges = 0;

  OMP("omp parallel") {
#if !defined(__MTA__)
    struct insqueue q;
    q.n = 0;
#endif

    OMP("omp for reduction(+: n_new_edges)")
      MTA("mta assert parallel")
      for (int64_t k = 0; k < nvlist; ++k) {
        const int64_t i = vlist[k];
        const int64_t ci = cmap[i];
        const int64_t mi = mark[i];

        assert (ci < new_nv);
        assert (mi < new_nv);
        assert (ci >= 0 || mi >= 0);

       /* fprintf (stderr, "deg(%ld) = %ld\n", (long)i, (long)stinger_outdegree_get (S, i)); */

        if (mi >= 0) {
          int64_t internal_cw = 0;
          int64_t internal_w = 0;
#if defined(__MTA__)
          struct insqueue q;
          q.n = 0;
#endif

          STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
            const int64_t j = STINGER_EDGE_DEST;
            const int64_t cj = cmap[j];
            const int64_t mj = mark[j];
            const int64_t w = STINGER_EDGE_WEIGHT;

            assert (cj < new_nv);
            assert (mj < new_nv);
            assert (cj >= 0 || mj >= 0);

           /* fprintf (stderr, "considering edge %ld (%ld) - %ld (%ld)\n", */
           /*       (long)i, (long)ci, (long)j, (long)cj); */

            /* Remove old edges. */
            if (ci != cj) {
              if (mj < 0 || ci < cj)
                ++n_new_edges;
            } else {
              /* Edge internal to community. */
              internal_cw += w;
            }

            /* Add new edges. */
            if (mi == mj) { /* XXX: (Currently, mi==mj when mi>=0 only if i==j) */
              internal_w += w;
            } else if (mj < 0) {
              /* Kept old community id, j won't be inserting edges */
              ++n_new_edges;
            } else {
              if (mi < mj) //((mi ^ mj) & 0x01)
                /* Ensure only one side adds the edge. */
                ++n_new_edges;
            }

          } STINGER_FORALL_EDGES_OF_VTX_END();

          if (internal_cw) D(g, ci) -= internal_cw;
          D(g, mi) = internal_w;
        }
      }

    OMP("omp single") {
      if (realloc_graph (g, g->nv, g->ne + n_new_edges))
        abort ();
    }

    OMP("omp for")
      MTA("mta assert parallel")
      for (int64_t k = 0; k < nvlist; ++k) {
        const int64_t i = vlist[k];
        const int64_t ci = cmap[i];
        const int64_t mi = mark[i];

        assert (ci < new_nv);
        assert (mi < new_nv);
        assert (ci >= 0 || mi >= 0);

        if (mi >= 0) {
#if defined(__MTA__)
          struct insqueue q;
          q.n = 0;
#endif
          STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
            const int64_t j = STINGER_RO_EDGE_DEST;
            const int64_t cj = cmap[j];
            const int64_t mj = mark[j];
            const int64_t w = STINGER_RO_EDGE_WEIGHT;

            assert (cj < new_nv);
            assert (mj < new_nv);
            assert (cj >= 0 || mj >= 0);

            /* Remove old edges. */
            if (ci != cj) {
              if (mj < 0 || ci < cj)
                enqueue (&q, ci, cj, -w, g);
            }

            /* Add new edges. */
            if (mj < 0) {
              /* Kept old community id, j won't be inserting edges */
              enqueue (&q, mi, cj, w, g);
            } else {
              if (mi < mj) //((mi ^ mj) & 0x01)
                /* Ensure only one side adds the edge. */
                enqueue (&q, mi, mj, w, g);
            }

          } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();

#if defined(__MTA__)
          qflush (&q, g);
#endif
        }
      }

#if !defined(__MTA__)
    qflush (&q, g);
#endif
  }
}

static void
commit_cmap_change (const int64_t nvlist, const int64_t * restrict vlist,
                    int64_t * restrict cmap,
                    int64_t * restrict mark)
{
  OMP("omp parallel for") MTA("mta assert nodep")
    for (int64_t k = 0; k < nvlist; ++k) {
      const int64_t i = vlist[k];
      if (mark[i] >= 0) {
        cmap[i] = mark[i];
        mark[i] = -1;
      }
    }
}

/*
  in: el, rowstart, rowend
  tmpmap = 0:el.nv-1
  contract (el, el.nv, tmpmap, &rowstart, &rowend, ws)
*/

void
update_el (struct el * el,
           int64_t * cmap,
           int64_t * csize,
           const struct stinger * S,
           const int64_t nvlist, const int64_t * restrict vlist,
           int64_t * mark,
           int64_t ** ws, size_t *wslen)
{
  int64_t new_nv;

  new_nv = el->nv;
  extract_vertices (nvlist, vlist, cmap, S, mark, &new_nv, csize);

  if (realloc_graph (el, new_nv, el->ne_orig))
    abort ();
  /* nactvtx_trace[ntrace] = new_nv; */

  if (el->nv != new_nv) fprintf (stderr, "WTF %ld %ld\n", (long)el->nv, (long)new_nv);
  assert (el->nv == new_nv);

  extract_edges (nvlist, vlist, cmap, S, mark, el, new_nv);
  /* nactel_trace[ntrace] = el->ne; */

  assert (el->nv == new_nv);

#if !defined(NDEBUG)
  {
    struct el g = *el;
    CDECL(g);
    for (int64_t k = 0; k < g.ne; ++k) {
      if (I(g, k) >= g.nv) fprintf (stderr, "ugh2 %ld (%ld, %ld; %ld) %ld\n", (long)k, (long)I(g,k), (long)J(g,k), (long)W(g,k), (long)g.nv);
      assert (I(g, k) < g.nv);
      assert (J(g, k) < g.nv);
      assert (I(g, k) >= 0);
      assert (J(g, k) >= 0);
    }
  }
#endif

  commit_cmap_change (nvlist, vlist, cmap, mark); /* mark now -1 */
#if !defined(NDEBUG)
  OMP("omp parallel for")
    for (int64_t k = 0; k < el->nv_orig; ++k)
      assert(mark[k] == -1);
#endif

#if !defined(NDEBUG)
  extern int64_t global_gwgt;
  global_gwgt = -1;
#endif
  realloc_ws (ws, wslen, el->nv_orig, el->ne_orig);
  contract_self (el, *ws);
}

MTA("mta inline") MTA("mta expect parallel context")
static inline int
append_to_vlist (int64_t * restrict nvlist,
                 int64_t * restrict vlist,
                 int64_t * restrict mark /* |V|, init to -1 */,
                 const int64_t i)
{
  int out = 0;

  /* for (int64_t k = 0; k < *nvlist; ++k) { */
  /*   assert (vlist[k] >= 0); */
  /*   if (k != mark[vlist[k]]) */
  /*     fprintf (stderr, "wtf mark[vlist[%ld] == %ld] == %ld\n", */
  /*              (long)k, (long)vlist[k], (long)mark[vlist[k]]); */
  /*   assert (mark[vlist[k]] == k); */
  /* } */

  if (mark[i] < 0) {
    int64_t where;
    if (bool_int64_compare_and_swap (&mark[i], -1, -2)) {
      assert (mark[i] == -2);
      /* Own it. */
      where = int64_fetch_add (nvlist, 1);
      mark[i] = where;
      vlist[where] = i;
      out = 1;
    }
  }

  return out;
}

void cstate_preproc (struct community_state * restrict cstate,
                     const struct stinger * S,
                     const int64_t nincr, const int64_t * restrict incr,
                     const int64_t nrem, const int64_t * restrict rem)
{
  const int64_t * restrict cmap = cstate->cmap;
  int64_t nvlist = cstate->nvlist;
  int64_t * restrict vlist = cstate->vlist;
  int64_t * restrict mark = cstate->mark;
  intvtx_t * restrict d = cstate->cg.d;
  int64_t n_new_edges = 0;

  OMP("omp parallel") {
    struct insqueue q;
    q.n = 0;

    OMP("omp for")
      for (int64_t k = 0; k < nvlist; ++k) mark[vlist[k]] = -1;
    OMP("omp single") nvlist = 0;

    OMP("omp for reduction(+: n_new_edges)")
      for (int64_t k = 0; k < nincr; ++k) {
        const int64_t i = incr[3*k+0];
        const int64_t j = incr[3*k+1];
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj)
          ++n_new_edges;
      }

    OMP("omp for reduction(+: n_new_edges)")
      for (int64_t k = 0; k < nrem; ++k) {
        const int64_t i = rem[2*k+0];
        const int64_t j = rem[2*k+1];
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj)
          ++n_new_edges;
      }

    OMP("omp single") {
      if (realloc_graph (&cstate->cg, cstate->cg.nv, cstate->cg.ne + n_new_edges))
       abort ();
    }

    OMP("omp for")
      for (int64_t k = 0; k < nincr; ++k) {
        const int64_t i = incr[3*k+0];
        const int64_t j = incr[3*k+1];
        const int64_t w = incr[3*k+2];
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj) {
          append_to_vlist (&nvlist, vlist, mark, i);
          append_to_vlist (&nvlist, vlist, mark, j);
          enqueue (&q, ci, cj, w, &cstate->cg);
        } else
          OMP("omp atomic") d[ci] += w;
      }

    OMP("omp for")
      for (int64_t k = 0; k < nrem; ++k) {
        const int64_t i = rem[2*k+0];
        const int64_t j = rem[2*k+1];
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj) {
          append_to_vlist (&nvlist, vlist, mark, i);
          append_to_vlist (&nvlist, vlist, mark, j);
        }
        /* Find the weight to remove.  ugh. */
        STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, i) {
          if (STINGER_EDGE_DEST == j) {
            if (ci != cj)
              enqueue (&q, ci, cj, -STINGER_EDGE_WEIGHT, &cstate->cg);
            else
              OMP("omp atomic") d[ci] -= STINGER_EDGE_WEIGHT;
            break;
            /* XXX: Technically, could have many of different types. */
          }
        } STINGER_FORALL_EDGES_OF_VTX_END ();
      }

    qflush (&q, &cstate->cg);
  }
  cstate->nvlist = nvlist;
  if (n_new_edges) {
    realloc_ws (&cstate->ws, &cstate->wslen, cstate->cg.nv_orig, cstate->cg.ne_orig);
    contract_self (&cstate->cg, cstate->ws);
  }
}

void cstate_preproc_acts (struct community_state * restrict cstate,
                          const struct stinger * S,
                          const int64_t nact, const int64_t * restrict act)
{
  const int64_t * restrict cmap = cstate->cmap;
  int64_t nvlist = cstate->nvlist;
  int64_t * restrict vlist = cstate->vlist;
  int64_t * restrict mark = cstate->mark;
  intvtx_t * restrict d = cstate->cg.d;
  int64_t n_new_edges = 0;

  OMP("omp parallel") {
    struct insqueue q;
    q.n = 0;

    OMP("omp for")
      for (int64_t k = 0; k < nvlist; ++k) mark[vlist[k]] = -1;
    OMP("omp single") nvlist = 0;

    OMP("omp for reduction(+: n_new_edges)")
      for (int64_t k = 0; k < nact; ++k) {
        int64_t i = act[2*k+0];
        int64_t j = act[2*k+1];
        const int isdel = i < 0;

        if (isdel) { i = ~i; j = ~j; }

        assert (i < cstate->graph_nv);
        assert (j < cstate->graph_nv);

        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];

        if (ci != cj)
          ++n_new_edges;
      }

    OMP("omp single") {
      if (realloc_graph (&cstate->cg, cstate->cg.nv, cstate->cg.ne + n_new_edges))
       abort ();
    }

    OMP("omp for")
      for (int64_t k = 0; k < nact; ++k) {
        int64_t i = act[2*k+0];
        int64_t j = act[2*k+1];
        const int64_t w = 1;
        const int isdel = i < 0;

        if (isdel) { i = ~i; j = ~j; }

        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];

        if ((!isdel && ci != cj) || (isdel && ci == cj)) {
          append_to_vlist (&nvlist, vlist, mark, i);
          assert (nvlist <= cstate->graph_nv);
          append_to_vlist (&nvlist, vlist, mark, j);
          assert (nvlist <= cstate->graph_nv);
        }

        if (!isdel) {
          if (ci != cj)
            enqueue (&q, ci, cj, w, &cstate->cg);
          else
            OMP("omp atomic") d[ci] += w;
        } else {
          /* Find the weight to remove.  ugh. */
          /* fprintf (stderr, "searching for weight of %ld %ld\n", (long)i, (long)j); */
          STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, i) {
            if (STINGER_EDGE_DEST == j) {
              if (ci != cj)
                enqueue (&q, ci, cj, -STINGER_EDGE_WEIGHT, &cstate->cg);
              else
                OMP("omp atomic") d[ci] -= w;
              break;
              /* XXX: Technically, could have many of different types. */
            }
          } STINGER_FORALL_EDGES_OF_VTX_END ();
        }
      }
    qflush (&q, &cstate->cg);
  }
  cstate->nvlist = nvlist;
  if (n_new_edges) {
    realloc_ws (&cstate->ws, &cstate->wslen, cstate->cg.nv_orig, cstate->cg.ne_orig);
    contract_self (&cstate->cg, cstate->ws);
  }
}

double
init_cstate_from_stinger (struct community_state * cs, const struct stinger * S)
{
  const int64_t nv = stinger_max_active_vertex(S) + 1;
  int64_t ne = stinger_max_total_edges(S);
  struct el g = alloc_graph (nv, ne);
  double time = 0;

  DECL(g);

  tic();
  OMP("omp parallel") {
    struct insqueue q;
    q.n = 0;

    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i)
        D(g, i) = 0;
    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i) {
       STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
         const int64_t j = STINGER_RO_EDGE_DEST;
         const int64_t w = STINGER_RO_EDGE_WEIGHT;
         if (i < j) {
            enqueue (&q, i, j, w, &g);
         } else if (i == j) {
           OMP("omp atomic") D(g, i) += w;
         }
       } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
      }
    qflush (&q, &g);
  }

  shrinkwrap_graph(&g);
  time = toc();

  time += init_and_compute_community_state (cs, &g);
  return time;
}

double
cstate_preproc_alg (struct community_state * restrict cstate,
                   const stinger_registered_alg * alg)
{
  const struct stinger * S = alg->stinger;
  const int64_t nincr = alg->num_insertions;
  const stinger_edge_update * restrict incr = alg->insertions;
  const int64_t nrem = alg->num_deletions;
  const stinger_edge_update * restrict rem = alg->deletions;
  const int64_t * restrict cmap = cstate->cmap;
  int64_t nvlist = cstate->nvlist;
  int64_t * restrict vlist = cstate->vlist;
  int64_t * restrict mark = cstate->mark;
  intvtx_t * restrict d = cstate->cg.d;
  int64_t n_new_edges = 0;

  tic ();
  OMP("omp parallel") {
    struct insqueue q;
    q.n = 0;

    OMP("omp for")
      for (int64_t k = 0; k < nvlist; ++k) mark[vlist[k]] = -1;
    OMP("omp single") nvlist = 0;

    OMP("omp for reduction(+: n_new_edges)")
      for (int64_t k = 0; k < nincr; ++k) {
        const int64_t i = incr[k].source;
        const int64_t j = incr[k].destination;
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj)
          ++n_new_edges;
      }

    OMP("omp for reduction(+: n_new_edges)")
      for (int64_t k = 0; k < nrem; ++k) {
        const int64_t i = rem[k].source;
        const int64_t j = rem[k].destination;
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj)
          ++n_new_edges;
      }

    OMP("omp single") {
      if (realloc_graph (&cstate->cg, cstate->cg.nv, cstate->cg.ne + n_new_edges))
       abort ();
    }

    OMP("omp for")
      for (int64_t k = 0; k < nincr; ++k) {
        const int64_t i = incr[k].source;
        const int64_t j = incr[k].destination;
        const int64_t w = incr[k].weight;
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj) {
          append_to_vlist (&nvlist, vlist, mark, i);
          append_to_vlist (&nvlist, vlist, mark, j);
          enqueue (&q, ci, cj, w, &cstate->cg);
        } else
          OMP("omp atomic") d[ci] += w;
      }

    OMP("omp for")
      for (int64_t k = 0; k < nrem; ++k) {
        const int64_t i = rem[k].source;
        const int64_t j = rem[k].destination;
        const int64_t ci = cmap[i];
        const int64_t cj = cmap[j];
        if (ci != cj) {
          append_to_vlist (&nvlist, vlist, mark, i);
          append_to_vlist (&nvlist, vlist, mark, j);
        }
        /* Find the weight to remove.  ugh. */
        STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, i) {
          if (STINGER_EDGE_DEST == j) {
            if (ci != cj)
              enqueue (&q, ci, cj, -STINGER_EDGE_WEIGHT, &cstate->cg);
            else
              OMP("omp atomic") d[ci] -= STINGER_EDGE_WEIGHT;
            break;
            /* XXX: Technically, could have many of different types. */
          }
        } STINGER_FORALL_EDGES_OF_VTX_END ();
      }

    qflush (&q, &cstate->cg);
  }
  cstate->nvlist = nvlist;
  if (n_new_edges) {
    realloc_ws (&cstate->ws, &cstate->wslen, cstate->cg.nv_orig, cstate->cg.ne_orig);
    contract_self (&cstate->cg, cstate->ws);
  }
  return toc();
}
