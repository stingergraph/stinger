#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <errno.h>
#include <assert.h>

#include "stinger_utils/timer.h"

#include "stinger_core/xmalloc.h"

#include "compat.h"
#include "sorts.h"
#include "graph-el.h"
#include "community.h"

#if !defined(NO_SCHED)
#define SCHED_STATIC " schedule(static)"
#define SCHED_GUIDED " schedule(guided)"
#endif

#undef MTA_STREAMS
#if !defined(MTA_STREAMS)
#define MTA_STREAMS MTA("mta use 100 streams")
#endif

#if defined(_OPENMP)
#define OMP_LOCKSPACE_DECL , omp_lock_t * restrict lockspace
#define OMP_LOCKSPACE_PARAM , lockspace
#else
#define OMP_LOCKSPACE_DECL
#define OMP_LOCKSPACE_PARAM
#endif

int global_verbose;
#if !defined(NDEBUG)
int64_t global_gwgt = -1;
#endif

#define LOAD_ORDERED(g_, k_, i_, j_)                      \
  do {                                                    \
    i_ = I(g_, k_);                                       \
    j_ = J(g_, k_);                                       \
    if (i_ > j_) { intvtx_t t_ = j_; j_ = i_; i_ = t_; }  \
  } while (0)

static int
is_maximal_p (const struct el g,
              int64_t * restrict m /* NV */,
              const double * restrict s);

static int64_t
calc_weight (const struct el g)
{
  int64_t weight_sum = 0;
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);
  OMP("omp parallel") {
    OMP("omp for reduction(+: weight_sum) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < nv; ++k)
        weight_sum += D(g, k);
    OMP("omp barrier");
    OMP("omp for reduction(+: weight_sum) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k)
        weight_sum += W(g, k);
  }
  return weight_sum;
}

static int64_t weight_sum;

static int64_t FN_MAY_BE_UNUSED
all_calc_weight_base (const int64_t nv, const int64_t ne,
                      const intvtx_t * restrict el,
                      const intvtx_t * restrict d,
                      const int64_t * restrict rowstart, const int64_t * restrict rowend)
{
  OMP("omp master")
    weight_sum = 0;
  OMP("omp barrier");
  OMP("omp for reduction(+:weight_sum) schedule(static)")
    for (int64_t i = 0; i < nv; ++i)
      weight_sum += d[i];
  OMP("omp for reduction(+:weight_sum) schedule(guided)")
    for (int64_t i = 0; i < nv; ++i)
      for (int64_t k = rowstart[i]; k < rowend[i]; ++k)
        weight_sum += Wel(el, k);
  return weight_sum;
}

static int64_t FN_MAY_BE_UNUSED
all_calc_weight_base_flat (const int64_t nv, const int64_t ne,
                           const intvtx_t * restrict el,
                           const intvtx_t * restrict d)
{
  OMP("omp master")
    weight_sum = 0;
  OMP("omp barrier");
  OMP("omp for reduction(+:weight_sum) schedule(static)")
    for (int64_t i = 0; i < nv; ++i)
      weight_sum += d[i];
  OMP("omp for reduction(+:weight_sum) schedule(static)")
    for (int64_t k = 0; k < ne; ++k)
      weight_sum += Wel(el, k);
  OMP("omp flush (weight_sum)");
  return weight_sum;
}

static intvtx_t
convert_el_match_to_relabel (const struct el g,
                             int64_t * restrict m /* NV */,
                             int64_t * restrict ws)
{
  const intvtx_t NV = g.nv;
  intvtx_t new_NV = 0;
  CDECL(g);

#if !defined(NDEBUG)
  int64_t * count = calloc (g.ne, sizeof (*count));
  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
      for (intvtx_t ki = 0; ki < NV; ++ki) {
        if (m[ki] >= 0) {
          const int64_t km = m[ki];
          const intvtx_t i = I(g, km);
          const intvtx_t j = J(g, km);
          /* if (i == 21691 || i == 17104 || j == 21691 || j == 17104 || */
          /*     i == 43381 || i == 10517 || j == 43381 || j == 10517 */
          /*     ) { */
          /*   fprintf (stderr, "m[%ld] = (%ld, %ld)  %ld\n", ki, i, j, km); */
          /* } */
          assert (i != j);
          assert (m[i] == km);
          assert (m[j] == km);
          int64_fetch_add (&count[km], 1);
        }
      }
    OMP("omp barrier");
    OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
      for (int64_t k = 0; k < g.ne; ++k) {
        if (count[k] < 0) fprintf (stderr, "%ld WTF?!?\n", k);
        assert (count[k] >= 0);
        if (!(count[k] == 0 || count[k] == 2)) {
          OMP("omp critical") {
            fprintf (stderr, "ughugh count[(%ld, %ld)]==%ld\n",
                     (long)I(g, k), (long)J(g, k), count[k]);
            for (intvtx_t ki = 0; ki < NV; ++ki) {
              if (m[ki] == k)
                fprintf (stderr, "  seen at m[%ld]\n", (long)ki);
            }
          }
        }
        assert (count[k] == 0 || count[k] == 2);
      }
  }
  free (count);
#endif

  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < NV; ++i)
        ws[i] = -1;
    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < NV; ++i) {
        if (m[i] >= 0) {
          const intvtx_t j = (i == I(g, m[i])? J(g, m[i]) : I(g, m[i]));
          assert (j < NV);
          if (i > j) { /* Ensure only one of the pair claims a number. */
            const intvtx_t newlbl = intvtx_fetch_add (&new_NV, 1);
            ws[i] = ws[j] = newlbl;
          } else if (i == j) {
            const intvtx_t newlbl = intvtx_fetch_add (&new_NV, 1);
            ws[i] = newlbl;
          }
        }
        else /* Negative, unmatched but kept. */
          ws[i] = intvtx_fetch_add (&new_NV, 1);
      }
    OMP("omp barrier");
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < NV; ++i)
        m[i] = ws[i];
#if !defined(NDEBUG)
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < new_NV; ++i)
        ws[i] = 0;
    OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
      for (intvtx_t i = 0; i < NV; ++i) {
        OMP("omp atomic") ++ws[m[i]];
        if (m[i] > new_NV)
          fprintf (stderr, "m[%ld] == %ld > %ld\n", (long)i, (long)m[i], (long)new_NV);
        assert (m[i] < new_NV);
        assert (m[i] >= 0);
      }
    OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
      for (intvtx_t i = 0; i < new_NV; ++i) {
        assert (ws[i] > 0);
        assert (ws[i] <= 2);
      }
#endif
  }
  return new_NV;
}

static intvtx_t
convert_el_tree_to_relabel (const struct el g,
                            int64_t * restrict m /* NV */,
                            int64_t * restrict ws)
{
  const intvtx_t NV = g.nv;
  intvtx_t new_NV = 0;
#if !defined(NDEBUG)
  int64_t nrt = 0;
#endif
  CDECL(g);

  /* The total ordering insures a tree.  The root occurs where the
     same edge is selected by two vertices.  Choose the lesser index
     as the root.  There is no general reason to prefer any particular
     vertex in the tree as a root. */
  OMP("omp parallel") {
    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki)
        ws[ki] = -1;

    /* Convert to a tree. */

    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki) {
        const int64_t k = m[ki];
        if (k >= 0) {
          const intvtx_t kj = (J(g, k) != ki? J(g, k) : I(g, k));
          assert (ki == I(g, k) || ki == J(g, k));
          if (k == m[kj] && ki < kj) {
            /* Then ki is the root. */
            ws[ki] = ki;
#if !defined(NDEBUG)
            OMP("omp atomic") ++nrt;
#endif
          } else /* Point towards the better side: away. */
            ws[ki] = kj;
        }
      }
    assert(nrt > 0);

    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki)
        m[ki] = -1;

    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki)
        if (ws[ki] >= 0 && ki == ws[ki]) /* At a root, assign a new index. */
          m[ki] = intvtx_fetch_add (&new_NV, 1);

    OMP("omp single")
      assert (nrt == new_NV);

    /* Spread new indices. */
    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki) {
        if (ws[ki] < 0) { /* Not in tree, mapped to new vertex. */
          assert (m[ki] < 0);
          m[ki] = intvtx_fetch_add (&new_NV, 1);
        } else if (m[ki] < 0) {
          intvtx_t k = ws[ki];
          while (m[k] < 0) {
            /* Benign, possibly helpful race... */
            k = ws[k];
            assert (k >= 0); /* Always in tree. */
          }
          m[ki] = m[k];
        }
      }

#if !defined(NDEBUG)
    OMP("omp for")
      for (intvtx_t ki = 0; ki < NV; ++ki) {
        if (m[ki] >= new_NV) {
          fprintf (stderr, "%ld >= %ld\n", (long)m[ki], (long)new_NV);
        }
        assert (m[ki] < new_NV);
      }
#endif
  }
  return new_NV;
}

#if defined(_OPENMP)
static int64_t *buf;

static int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  int nt, tid;
  int64_t slice_begin, slice_end, t1, t2, k, tmp;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();
  OMP("omp master")
    buf = alloca (nt * sizeof (*buf));
  OMP("omp flush (buf)");
  OMP("omp barrier");

  t1 = n / nt;
  t2 = n % nt;
  slice_begin = t1 * tid + (tid < t2? tid : t2);
  slice_end = t1 * (tid+1) + ((tid+1) < t2? (tid+1) : t2);

  tmp = 0;
  for (k = slice_begin; k < slice_end; ++k)
    tmp += ary[k];
  buf[tid] = tmp;
  OMP("omp barrier");
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  OMP("omp barrier");
  tmp = buf[nt-1];
  if (tid)
    t1 = buf[tid-1];
  else
    t1 = 0;
  for (k = slice_begin; k < slice_end; ++k) {
    int64_t t = ary[k];
    ary[k] = t1;
    t1 += t;
  }
  OMP("omp barrier");
  return t1;
}
#else
MTA("mta inline")
static int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  int64_t cur = 0;
  for (int64_t k = 0; k < n; ++k) {
    int64_t tmp = ary[k];
    ary[k] = cur;
    cur += tmp;
  }
  return cur;
}
#endif

static void
rough_bucket_sort_el (const intvtx_t nv, const int64_t ne,
                      const intvtx_t * restrict el,
                      int64_t * restrict off,
                      intvtx_t * restrict tailend)
{
  OMP("omp barrier");
  OMP("omp for schedule(static)") MTA_STREAMS
    for (intvtx_t k = 0; k <= nv; ++k)
      off[k] = 0;
  OMP("omp for schedule(static)") MTA_STREAMS
    for (int64_t k = 0; k < ne; ++k)
      int64_fetch_add (&off[1+Iel(el,k)], 1);
  prefix_sum (nv, &off[1]);
  OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
    for (int64_t k = 0; k < ne; ++k) {
      intvtx_t i = Iel(el, k);
      int64_t loc = int64_fetch_add (&off[1+i], 1);
      tailend[2*loc] = Jel(el, k);
      tailend[1+2*loc] = Wel(el, k);
    }
  assert (off[nv] == ne);
#if !defined(NDEBUG)
  OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
    for (intvtx_t k = 0; k < nv; ++k) {
      if (off[k] > off[k+1]) {
        fprintf (stderr, "ugh %ld  [%ld, %ld)\n", (long)k, off[k], off[k+1]);
      }
      assert (off[k] <= off[k+1]);
    }
#endif
}

static void
rough_bucket_copy_back (const intvtx_t nv,
                        int64_t * restrict n_new_edges,
                        intvtx_t * restrict el,
                        int64_t * restrict rowstart,
                        int64_t * restrict rowend,
                        const int64_t * restrict off,
                        const int64_t * restrict offend, /* cheating... */
                        const intvtx_t * restrict tailend)
{
  OMP("omp master")
    *n_new_edges = 0;
  OMP("omp barrier");
  OMP("omp for schedule(guided)") MTA("mta assert parallel") MTA_STREAMS
    for (intvtx_t new_i = 0; new_i < nv; ++new_i) {
      const int64_t start_src = off[new_i];
      const int64_t n_to_copy = offend[new_i] - off[new_i];
      const int64_t start = int64_fetch_add (n_new_edges, n_to_copy);
      /* if (new_i < 10 || *n_new_edges < 1) */
      /*  fprintf (stderr, "%ld/%ld [%ld, %ld): %ld %ld   %ld\n", new_i, nv, off[new_i], offend[new_i], n_to_copy, start, *n_new_edges); */
      /* if (new_i < 10) fprintf (stderr, "new row %ld deg %ld\n", new_i, n_to_copy); */
      for (int64_t k = 0; k < n_to_copy; ++k) {
        Iel(el, start+k) = new_i;
        Jel(el, start+k) = tailend[2*(start_src+k)];
        Wel(el, start+k) = tailend[1+2*(start_src+k)];
      }
      rowstart[new_i] = start;
      rowend[new_i] = start + n_to_copy;
    }
}

static int64_t
contract_el (int64_t NE, intvtx_t * restrict el /* 3 x oldNE */,
             intvtx_t old_nv,
             intvtx_t new_nv,
             const int64_t * restrict m /* oldNV */,
             intvtx_t * restrict d /* oldNV */,
             int64_t * restrict rowstart /* newNV */,
             int64_t * restrict rowend /* newNV */,
             int64_t * restrict ws /* newNV+1 + 2*oldNE */)
{
  int64_t n_new_edges = 0;

#if !defined(NDEBUG)
  int64_t w_in = 0, w_out = 0;
  int64_t cutw_in = 0, cutw_out = 0;
#endif

  int64_t * restrict count = ws;
  intvtx_t * restrict tmpcopy = (intvtx_t*)&count[new_nv+1];

  /* fprintf (stderr, "from %ld => %ld\n", old_nv, new_nv); */

  OMP("omp parallel") 
  {
#if !defined(NDEBUG)
    OMP("omp for reduction(+:w_in) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < old_nv; ++i)
        w_in += d[i];
    OMP("omp for reduction(+:w_in) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < NE; ++k)
        w_in += Wel(el, k);
    OMP("omp barrier");
    w_out = all_calc_weight_base_flat (old_nv, NE, el, d);
    if (global_gwgt >= 0 && w_in != global_gwgt) {
      fprintf (stderr, "%d/%d: %ld != %ld   %ld\n", omp_get_thread_num ()+1, omp_get_num_threads (),
               w_in, global_gwgt, w_out);
    }
    OMP("omp master") assert (global_gwgt < 0 || w_in == global_gwgt);
    assert (w_out == w_in);
    OMP("omp for reduction(+:cutw_in) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < NE; ++k) {
        const intvtx_t i = Iel(el, k);
        const intvtx_t j = Jel(el, k);
        if (m[i] != m[j])
          cutw_in += Wel(el, k);
      }
    assert (cutw_in <= w_in);
#endif

    /* Collapse existing diagonal. */
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i <= new_nv; ++i)
        count[i] = 0;
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t old_i = 0; old_i < old_nv; ++old_i) {
        const intvtx_t new_i = m[old_i];
        assert (m[old_i] < new_nv);
        if (new_i >= 0)
          OMP("omp atomic") MTA("mta update") count[new_i] += d[old_i];
      }
    OMP("omp for") MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i) {
        d[i] = count[i];
        assert (count[i] <= INTVTX_MAX);
        count[i] = 0;
      }

#if !defined(NDEBUG)
    int64_t lw_out = all_calc_weight_base_flat (new_nv, NE, el, d);
    OMP("omp master")
      w_out = lw_out;
    OMP("omp barrier");
    OMP("omp single")
      if (w_in != w_out)
        fprintf (stderr, "%ld %ld %ld\n", (long)w_in, (long)w_out, (long)global_gwgt);
    OMP("omp barrier");
    OMP("omp master")
      assert (w_out == w_in);
    OMP("omp barrier");
    OMP("omp single") w_out = 0;
#endif

    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (int64_t k = 0; k < NE; ++k) {
#if !defined(NDEBUG)
        intvtx_t old_i = Iel(el, k);
        intvtx_t old_j = Jel(el, k);
#endif
        intvtx_t new_i = m[Iel(el, k)];
        intvtx_t new_j = m[Jel(el, k)];
        assert (m[Iel(el, k)] <= INTVTX_MAX);
        assert (m[Jel(el, k)] <= INTVTX_MAX);
        assert (old_i < old_nv);
        assert (old_j < old_nv);
        assert (new_i < new_nv);
        assert (new_j < new_nv);
        canonical_order_edge (&new_i, &new_j);
        Iel(el, k) = new_i;
        Jel(el, k) = new_j;
      }

    rough_bucket_sort_el (new_nv, NE, el, count, tmpcopy);
    assert (NE == count[new_nv]);

#if !defined(NDEBUG)
    OMP("omp master") {
      w_out = 0;
      cutw_out = 0;
    }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)")  MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i)
        w_out += d[i];
    OMP("omp for reduction(+:w_out, cutw_out) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i) {
        for (int64_t k = count[i]; k < count[1+i]; ++k) {
          w_out += tmpcopy[1+2*k];
          if (i != tmpcopy[2*k]) cutw_out += tmpcopy[1+2*k];
        }
      }
    OMP("omp master") {
      if (w_in != w_out) {
        fprintf (stderr, "%d: w_in %ld w_out %ld\n",
#if defined(_OPENMP)
                 omp_get_thread_num (),
#else
                 1,
#endif
                 w_in, w_out);
      }
      assert (w_in == w_out);

      if (cutw_in != cutw_out) {
        fprintf (stderr, "%d: cutw_in %ld cutw_out %ld\n",
#if defined(_OPENMP)
                 omp_get_thread_num (),
#else
                 1,
#endif
                 cutw_in, cutw_out);
      }
      assert (cutw_in == cutw_out);
    }
#endif

    /* int64_t dumped_row = 0; */
    /* Sort then collapse within each row. */
    OMP("omp for schedule(guided)") MTA("mta assert parallel") MTA_STREAMS
      for (intvtx_t new_i = 0; new_i < new_nv; ++new_i) {
        const int64_t new_i_end = count[new_i+1];
        int64_t k, kcur = count[new_i];
#if !defined(NDEBUG)
        int64_t diagsum = 0, row_w_in = 0, row_w_out = 0;
        for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2)
          row_w_in += tmpcopy[1+2*k2];
#endif
        //if (dumped_row < 2 && new_i_end - count[new_i] > 3) {
        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld A:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) */
        /*    fprintf (stderr, " (%ld; %ld)", tmpcopy[2*k2], tmpcopy[1+2*k2]); */
        /*	fprintf (stderr, "\n"); */
        /* } */

        introsort_iki (&tmpcopy[2*count[new_i]], new_i_end - count[new_i]);
#if !defined(NDEBUG)
        /* fprintf (stderr, " %ld", tmpcopy[2*count[new_i]]); */
        for (int64_t k2 = count[new_i]+1; k2 < new_i_end; ++k2) {
          /* fprintf (stderr, " %ld", tmpcopy[2*k2]); */
          assert (tmpcopy[2*k2] >= tmpcopy[2*(k2-1)]);
        }
        /* fprintf (stderr, "\n"); */
#endif

        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld B:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) */
        /*    fprintf (stderr, " (%ld; %ld)", tmpcopy[2*k2], tmpcopy[1+2*k2]); */
        /*	fprintf (stderr, "\n"); */
        /* } */

        for (k = count[new_i]; k < new_i_end; ++k) {
          if (new_i == tmpcopy[2*k]) {
#if !defined(NDEBUG)
            diagsum += tmpcopy[1+2*k];
#endif
            d[new_i] += tmpcopy[1+2*k];
          } else
            break;
        }

        kcur = count[new_i];

        if (k == new_i_end) {
          /* Left with an empty row. */
          rowend[new_i] = kcur;
        } else {
          if (k != kcur) {
            tmpcopy[2*kcur] = tmpcopy[2*k];
            tmpcopy[1+2*kcur] = tmpcopy[1+2*k];
          }
          for (++k; k < new_i_end; ++k) {
            if (new_i == tmpcopy[2*k]) {
#if !defined(NDEBUG)
              diagsum += tmpcopy[1+2*k];
#endif
              d[new_i] += tmpcopy[1+2*k];
            } else if (tmpcopy[2*k] != tmpcopy[2*kcur]) {
              ++kcur;
              if (kcur != k) {
                tmpcopy[2*kcur] = tmpcopy[2*k];
                tmpcopy[1+2*kcur] = tmpcopy[1+2*k];
              }
            } else
              tmpcopy[1+2*kcur] += tmpcopy[1+2*k];
          }
          rowend[new_i] = kcur+1;
        }

        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld C:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) { */
        /*    char *sep = " "; */
        /*    char *sep2 = ""; */
        /*    if (k2 == kcur) sep2 = sep = "|"; */
        /*    fprintf (stderr, "%s(%ld; %ld)%s", sep, tmpcopy[2*k2], tmpcopy[1+2*k2], sep2); */
        /*	} */
        /*	fprintf (stderr, "\n"); */
        /*	++dumped_row; */
        /* } */

#if !defined(NDEBUG)
        row_w_out = diagsum;
        for (int64_t k2 = count[new_i]; k2 < rowend[new_i]; ++k2)
          row_w_out += tmpcopy[1+2*k2];
        if (row_w_in != row_w_out)
          fprintf (stderr, "row %ld mismatch %ld => %ld   %ld\n", (long)new_i,
                   row_w_in, row_w_out, diagsum);
#endif
      }

#if !defined(NDEBUG)
    OMP("omp master") w_out = 0;
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i) {
        w_out += d[i];
      }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i) {
        for (int64_t k = count[i]; k < rowend[i]; ++k)
          w_out += tmpcopy[1+2*k];
      }
    OMP("omp master")
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    assert (w_in == w_out);
#endif

    rough_bucket_copy_back (new_nv, &n_new_edges, el,
                            rowstart, rowend,
                            count, rowend, tmpcopy);

#if !defined(NDEBUG)
    //w_out = all_calc_weight_base_flat (new_nv, n_new_edges, el, d);
    OMP("omp master") w_out = 0;
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < new_nv; ++i) {
        w_out += d[i];
      }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < n_new_edges; ++k) {
        assert (Iel(el, k) != Jel(el, k));
        w_out += Wel(el, k);
      }
    OMP("omp master") {
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    }
    assert (w_in == w_out);
#endif

#if !defined(NDEBUG)
    w_out = all_calc_weight_base (new_nv, n_new_edges, el, d,
                                  rowstart, rowend);
    OMP("omp master") {
      cutw_out = 0;
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    }
    OMP("omp barrier");
    assert (w_in == w_out);
    OMP("omp for reduction(+:cutw_out) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < n_new_edges; ++k)
        /* Only cut edges remain. */
        cutw_out += Wel(el, k);
    assert (cutw_out == cutw_in);
#endif
  }

  assert (n_new_edges <= NE);
  return n_new_edges;
}

void
contract (struct el * restrict g,
          const int64_t new_nv,
          int64_t * restrict m /* NV */,
          int64_t * restrict new_rowstart,
          int64_t * restrict new_rowend,
          int64_t * restrict ws /* NV+1 + 2*NE */)
{
  const intvtx_t nv = g->nv;
  const int64_t ne = g->ne;
  int64_t new_ne;
#if !defined(NDEBUG)
  int64_t w_in, w_out;
  w_in = calc_weight (*g);
#endif

#if !defined(NDEBUG)
  OMP("omp parallel for") MTA("mta assert parallel") MTA_STREAMS
    for (int64_t i = 0; i < nv; ++i)
      assert (m[i] < new_nv);
#endif
  new_ne = contract_el (ne, g->el, nv, new_nv, m, g->d,
                        new_rowstart, new_rowend, ws);
  g->nv = new_nv;
  g->ne = new_ne;
#if !defined(NDEBUG)
  w_out = calc_weight (*g);
  assert (w_in == w_out);
#endif
}

#if defined(USE_FC)
#define CMP_BETTER(i, j, score, k, s, g) (k < 0 || (s[k] < score || (s[k] == score && (D(g, i) < D(g, I(g, k)) || D(g, j) < D(g, J(g, k))))))
#else
#define CMP_BETTER(i, j, score, k, s, g) (k < 0 || (s[k] < score || (s[k] == score && (i < I(g, k) || (i == I(g, k) && j < J(g, k))))))
#endif

#define noOMP(x)

static intvtx_t
sliced_non_maximal_pass (const struct el g,
                         const int64_t * restrict rowstart,
                         const int64_t * restrict rowend,
                         intvtx_t nunmatched,
                         intvtx_t * restrict unmatched,
                         int64_t * restrict m /* NV */,
                         const double * restrict s,
                         int64_t * restrict bestm,
                         intvtx_t * restrict tmp
                         OMP_LOCKSPACE_DECL
                         )
{
  CDECL(g);
  intvtx_t new_nunmatched = 0;

  OMP("omp parallel") {
    /* Assume that m[i] < 0 when unmatched, == k when edge k is the best match. */
    OMP("omp for schedule(guided)") MTA_NODEP MTA_STREAMS
      for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
        const intvtx_t i = unmatched[ki];
        const int64_t kend = rowend[i];
        int64_t best_match = -1;
        double best_score = 0;
        assert (m[i] < 0);
        /* Find best unmatched neighbor. */
        for (int64_t k = rowstart[i]; k < kend; ++k) {
          const intvtx_t j = J(g, k);
          if (m[j] < 0 && s[k] > 0) {
            /* if ((i == 0 || j == 0) && (i < 20 && j < 20)){ */
            /*   fprintf (stderr, "eligible %ld %ld : %g\n", i, j, s[k]); */
            /* } */
            const double score = s[k];
            if (score > best_score) {
              best_match = k;
              best_score = score;
            }
          }
        }
        bestm[ki] = best_match;
      }

#if !defined(NDEBUG)
    OMP("omp for schedule(static)") MTA("mta assert parallel") MTA_STREAMS
      for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
        const intvtx_t i = unmatched[ki];
        const int64_t best_match = bestm[ki];
        assert (m[i] < 0);
        if (best_match < 0) continue;
        assert (i == I(g, best_match) || i == J(g, best_match));
        const intvtx_t j = (i == I(g, best_match)? I(g, best_match) : J(g, best_match));
        assert (m[j] < 0);
        assert (s[best_match] > 0);
      }
#endif

    OMP("omp for schedule(guided)") MTA_NODEP MTA_STREAMS
      for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
        intvtx_t i = unmatched[ki];
        const int64_t ke = bestm[ki];
        /* fprintf (stderr, "trying %ld  %ld\n", i, ke); */
        if (ke >= 0) {
          int done = 0;
          intvtx_t j = (i == I(g, ke)? J(g, ke) : I(g, ke));
          const double score = s[ke];
          if (i < j) {
            intvtx_t t = j;
            j = i;
            i = t;
          }
#if defined(__MTA__)
          do {
            int win_i = 0, win_j = 0;
            int64_t mi = m[i]; //readfe (&m[i]);
            if (mi < 0 || score > s[mi]) {
              win_i = 1;
            } else if (0 && score == s[mi]) {
              intvtx_t other_i = I(g, mi);
              intvtx_t other_j = J(g, mi);
              if (other_i < other_j) {
                intvtx_t t = other_j;
                other_j = other_i;
                other_i = t;
              }
              if (i < other_i || (i == other_i && j < other_j))
                win_i = 1;
            }
            if (win_i) {
              int64_t mj = readfe (&m[j]);
              if (mj < 0 || score > s[mj]) {
                win_j = 1;
              } else if (0 && score == s[mj]) {
                intvtx_t other_i = I(g, mj);
                intvtx_t other_j = J(g, mj);
                if (other_i < other_j) {
                  intvtx_t t = other_j;
                  other_j = other_i;
                  other_i = t;
                }
                if (i < other_i || (i == other_i && j < other_j))
                  win_j = 1;
              }
              if (win_j) {
                int64_t mi_new = readfe (&m[i]);
                if (mi == mi_new) {
                  mi_new = mj = ke;
                  done = 1;
                }
                writeef (&m[i], mi_new);
              } else
                done = 1;
              writeef (&m[j], mj);
            } else
              done = 1;
          } while (!done);
#else
          do {
            int win_i = 0, win_j = 0;
            int64_t mi, mj;
            mi = m[i];
            if (mi < 0 || score > s[mi]) {
              win_i = 1;
            } else if (score == s[mi]) {
              intvtx_t other_i = I(g, mi);
              intvtx_t other_j = J(g, mi);
              if (other_i < other_j) {
                intvtx_t t = other_i;
                other_i = other_j;
                other_j = t;
              }
              if (i < other_i || (i == other_i && j < other_j))
                win_i = 1;
            }
            if (win_i) {
              mj = m[j];
              if (mj < 0 || score > s[mj]) {
                win_j = 1;
              } else if (score == s[mj]) {
                intvtx_t other_i = I(g, mj);
                intvtx_t other_j = J(g, mj);
                if (other_i < other_j) {
                  intvtx_t t = other_i;
                  other_i = other_j;
                  other_j = t;
                }
                if (i < other_i || (i == other_i && j < other_j))
                  win_j = 1;
              }
            }
            if (win_j) {
              if (mi == m[i] && mj == m[j]) {
#if defined(_OPENMP)
                omp_set_lock (&lockspace[i]);
                omp_set_lock (&lockspace[j]);
#endif
                if (mi == m[i] && mj == m[j]) {
                  m[i] = m[j] = ke;
                  done = 1;
                }
#if defined(_OPENMP)
                omp_unset_lock (&lockspace[j]);
                omp_unset_lock (&lockspace[i]);
#endif
              }
            } else
              done = 1;
          } while (!done);
#endif
        }
      }

    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
        //const int64_t i = unmatched[ki];
        const int64_t ke = bestm[ki];
        if (ke >= 0) {
          //const int64_t j = (i == I(g, ke)? J(g, ke) : I(g, ke));
          const intvtx_t i = I(g, ke);
          const intvtx_t j = J(g, ke);
          if (m[i] == ke && m[j] != ke)
            m[i] = -1;
          else if (m[i] != ke && m[j] == ke)
            m[j] = -1;
        }
      }

    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
        const intvtx_t i = unmatched[ki];
        if (bestm[ki] >= 0 && m[i] < 0) {
          const intvtx_t loc = intvtx_fetch_add (&new_nunmatched, 1);
          /* const int64_t best_match = bestm[ki]; */
          /* if (best_match >= 0 && best_match != m[i]) { */
          tmp[loc] = i;
        }
      }

    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t ki = 0; ki < new_nunmatched; ++ki)
        unmatched[ki] = tmp[ki];

#if !defined(NDEBUG)
    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t ki = 0; ki < new_nunmatched; ++ki)
        assert (m[unmatched[ki]] < 0);
    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < g.nv; ++i) {
        if (m[i] >= 0) {
          if (m[i] != m[J(g, m[i])] || m[i] != m[I(g, m[i])]) {
            const intvtx_t j = J(g, m[i]);
            const intvtx_t actual_i = I(g, m[i]);
            fprintf (stderr, "from %ld, attempted %ld = (%ld, %ld)[%g]\n",
                     (long)i, m[i], (long)actual_i, (long)j, s[m[i]]);
            fprintf (stderr, "    m[%ld] = %ld (%ld, %ld)[%g], m[%ld] = %ld (%ld, %ld)[%g]\n",
                     (long)actual_i, m[actual_i], (long)I(g, m[actual_i]), (long)J(g, m[actual_i]), s[m[actual_i]],
                     (long)j, m[j], (long)I(g, m[j]), (long)J(g, m[j]), s[j]);
          }
          assert (i == I(g, m[i]) || i == J(g, m[i]));
          assert (m[i] == m[J(g, m[i])]);
          assert (m[i] == m[I(g, m[i])]);
        }
      }
#endif
  }

  /* fprintf (stderr, "new_nunmatched %ld\n", (int64_t)new_nunmatched); */
  return new_nunmatched;
}

static int FN_MAY_BE_UNUSED
any_adj_unmatched (const struct el g,
                   const int64_t * restrict rowstart,
                   const int64_t * restrict rowend,
                   const intvtx_t nunmatched,
                   const intvtx_t * restrict unmatched,
                   const int64_t * restrict m /* NV */)
{
  CDECL(g);
  int out = 0;
  OMP("omp parallel for") MTA("mta assert nodep") MTA_STREAMS
    for (intvtx_t ki = 0; ki < nunmatched; ++ki) {
      const intvtx_t i = unmatched[ki];
      assert (m[i] < 0);
      if (!out)
        for (int64_t k = rowstart[i]; k < rowend[i]; ++k) {
          if (m[J(g,k)] < 0) {
            out = 1;
            nonMTA_break;
          }
        }
    }
  return out;
}

int FN_MAY_BE_UNUSED
is_maximal_p (const struct el g,
              int64_t * restrict m /* NV */,
              const double * restrict s)
{
  CDECL(g);
  const int64_t NE = g.ne;
  int out = 1;
  MTA_STREAMS
    for (int64_t k = 0; k < NE; ++k) {
      if (s[k] > 0) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);
        if (m[i] < 0 && m[j] < 0) {
          /* m[i] = m[j] = k; */
          out = 0;
          /* fprintf (stderr, "edge (%ld, %ld; %g) eligible and unmatched\n", i, j, s[k]); */
        }
      }
    }
  /* fprintf (stderr, "%d/%d: is_maximal_p : %d\n", omp_get_thread_num () + 1, omp_get_num_threads (), out); */
  return out;
}

static int64_t
maximal_match_iter (const struct el g,
                    const int64_t * restrict rowstart,
                    const int64_t * restrict rowend,
                    int64_t * restrict m /* NV */,
                    const double * restrict s,
                    int64_t * ws
                    OMP_LOCKSPACE_DECL
                    )
{
  const intvtx_t NV = g.nv;
  intvtx_t nunmatched = NV;
  int64_t * restrict bestm = ws;
  intvtx_t * restrict unmatched = (intvtx_t*)&bestm[NV];
  intvtx_t * restrict tmp = &unmatched[NV];
  CDECL(g);

  OMP("omp parallel for") MTA_STREAMS
    for (intvtx_t i = 0; i < NV; ++i) {
      m[i] = -3;
      unmatched[i] = i;
    }

  nunmatched = NV;

  do {
    nunmatched = sliced_non_maximal_pass (g, rowstart, rowend,
                                          nunmatched, unmatched,
                                          m, s, bestm, tmp
                                          OMP_LOCKSPACE_PARAM);
    /* fprintf (stderr, "\t (%ld => %ld) \n", nunmatched_prev, nunmatched); */
    /* } while (!is_maximal_p (g, m, s)); */
    /* } while (nunmatched != nunmatched_prev); */
  } while (nunmatched);

#if !defined(NDEBUG)
  OMP("omp parallel for") MTA("mta assert parallel") MTA_STREAMS
    for (intvtx_t i = 0; i < NV; ++i) {
      if (m[i] >= 0) {
        const int64_t km = m[i];
        const intvtx_t actual_i = I(g, km);
        const intvtx_t j = J(g, km);
        assert (i == actual_i || i == j);
        if (m[actual_i] != km) {
          fprintf (stderr, "m[i]=m[%ld]=%ld m[actual_i]=m[%ld]=%ld m[j]=m[%ld]=%ld\n",
                   (long)i, m[i], (long)actual_i, m[actual_i], (long)j, m[j]);
        }
        assert (m[actual_i] == km);
        assert (m[j] == km);
      }
    }
  assert (is_maximal_p (g, m, s));
#endif

  return nunmatched;
}

static void
decimate_matching (const struct el g,
                   const int64_t * restrict rowstart,
                   const int64_t * restrict rowend,
                   int64_t * restrict m /* NV */,
                   const double * restrict s)
{
  const int64_t nv = g.nv;
  CDECL(g);

  OMP("omp parallel") {
    OMP("omp for schedule(guided)") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i) {
        int64_t mi = m[i];
        double smi = (mi >= 0? s[mi] : 0.0);
        if (m[i] >= 0) {
          for (int64_t k = rowstart[i]; k < rowend[i]; ++k) {
            const intvtx_t j = J(g, k);
            int64_t mj = m[j];
            if (mj >= 0) {
              double smj = s[mj];
              if (smi < smj) {
                m[i] = -1;
                nonMTA_break;
              } else if (smj == smi) {
                const intvtx_t mi_i = (i < j? i : j);
                const intvtx_t mi_j = (i < j? j : i);
                const intvtx_t mj_i = (I(g, mj) < J(g, mj)? I(g, mj) : J(g, mj));
                const intvtx_t mj_j = (I(g, mj) < J(g, mj)? J(g, mj) : I(g, mj));
                if (mi_i > mj_i || (mi_i == mj_i && mi_j > mj_j)) {
                  m[i] = -1;
                  nonMTA_break;
                }
              } else {
                m[j] = -1;
                nonMTA_break;
              }
            }
          }
        }
      }
    OMP("omp for schedule(static)") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i) {
        const int64_t mi = m[i];
        if (mi >= 0) {
          const intvtx_t j = (I(g, mi) == i? J(g, mi) : I(g, mi));
          if (m[j] < 0) m[i] = -1;
        }
      }
  }
}

MTA("mta inline")
MTA("mta expect parallel")
static inline void
store_best_tree_edge (const struct el g,
                      const intvtx_t i_tgt,
                      const intvtx_t best_i,
                      const intvtx_t best_j,
                      const double best_score,
                      const int64_t best_k,
                      int64_t * restrict m /* NV */,
                      const double * restrict s
                      OMP_LOCKSPACE_DECL
                      )
{
  CDECL(g);
  int done = 0;

  while (!done) {
    int64_t m_i_tgt, m_i_tgt_ini;
    m_i_tgt_ini = m[i_tgt];
    m_i_tgt = m_i_tgt_ini;

    if (m_i_tgt < 0)
      m_i_tgt = best_k;
    else {
      intvtx_t test_i, test_j;
      LOAD_ORDERED(g, m_i_tgt, test_i, test_j);
      if (s[m_i_tgt] < best_score ||
          (s[m_i_tgt] == best_score && (best_i < test_i ||
                                        (best_i == test_i && best_j < test_j)))) {
        /* Take it... */
        m_i_tgt = best_k;
      }
    }

    if (m[i_tgt] == m_i_tgt_ini) {
#if defined(_OPENMP)
      omp_set_lock (&lockspace[i_tgt]);
#endif
      int64_t m_i = readfe (&m[i_tgt]);
      if (m_i == m_i_tgt_ini) {
        m_i = m_i_tgt;
        done = 1;
      }
      writeef (&m[i_tgt], m_i);
#if defined(_OPENMP)
      omp_unset_lock (&lockspace[i_tgt]);
#endif
    }
  }

  assert (i_tgt == I(g, m[i_tgt]) || i_tgt == J(g, m[i_tgt]));
}

static void
simple_tree_cover (const struct el g,
                   const int64_t * restrict rowstart,
                   const int64_t * restrict rowend,
                   int64_t * restrict m /* NV */,
                   const double * restrict s,
                   int64_t * ws
                   OMP_LOCKSPACE_DECL
                   )
{
  const int64_t nv = g.nv;
  CDECL(g);

  OMP("omp parallel") {
    OMP("omp for")
      for (intvtx_t i = 0; i < nv; ++i)
        m[i] = -1;
    /* Every vertex identifies its best adjacent edge. */
    /* First locally. */
    OMP("omp for nowait")
      for (intvtx_t i = 0; i < nv; ++i) {
        const int64_t rowend_i = rowend[i];
        intvtx_t best_i = -1;
        intvtx_t best_j = -1;
        double best_score = -1;
        int64_t best_k = -1;
        for (int64_t k = rowstart[i]; k < rowend_i; ++k) {
          const double test_score = s[k];
          intvtx_t test_i, test_j;
          assert (i == I(g, k));
          LOAD_ORDERED(g, k, test_i, test_j);
          if (test_score > best_score ||
              (test_score == best_score && (test_i < best_i ||
                                            (test_i == i &&
                                             test_j < best_j)))) {
            best_k = k;
            best_score = test_score;
            best_i = test_i;
            best_j = test_j;
          }
        }
        if (best_k > 0) {
#if !defined(NDEBUG)
          intvtx_t dbg_i, dbg_j;
          LOAD_ORDERED (g, best_k, dbg_i, dbg_j);
          assert (best_i == dbg_i);
          assert (best_j == dbg_j);
          assert (i == best_i || i == best_j);
#endif
          store_best_tree_edge (g, i, best_i, best_j, best_score, best_k,
                                m, s OMP_LOCKSPACE_PARAM);
        }
      }

    /* Then scattering the checks to the other side. */
    OMP("omp for")
      for (intvtx_t i = 0; i < nv; ++i) {
        const int64_t rowend_i = rowend[i];
        for (int64_t k = rowstart[i]; k < rowend_i; ++k) {
          intvtx_t j = J(g, k);
          intvtx_t test_i, test_j;
          if (i < j) {
            test_i = i;
            test_j = j;
          } else {
            test_i = j;
            test_j = i;
          }

          store_best_tree_edge (g, j, test_i, test_j, s[k], k,
                                m, s OMP_LOCKSPACE_PARAM);
        }
      }
  }

#if !defined(NDEBUG)
  OMP("omp parallel for")
    for (intvtx_t i = 0; i < nv; ++i) {
      if (m[i] >= 0) {
        assert (i == I(g, m[i]) || i == J(g, m[i]));
      }
    }
#endif
}

static void
score_heaviest_edge (double * restrict escore,
                     const struct el g)
{
  CDECL(g);

  OMP("omp parallel for")
    MTA_NODEP
    MTA_STREAMS
    for (int64_t k = 0; k < g.ne; ++k) {
      escore[k] = W(g, k);
      assert (!isnan (escore[k]));
    }
}

static void
score_conductance (double * restrict escore,
                   const struct el g,
                   int64_t * ws)
{
  CDECL(g);
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  int64_t * restrict extdeg = ws;
  int64_t * restrict alldeg = &extdeg[nv];
  double * restrict old_conductance = (double*)&alldeg[nv];

  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        extdeg[i] = 0;
    OMP("omp for schedule(static)") MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        int64_fetch_add (&extdeg[I(g, k)], W(g, k));
        int64_fetch_add (&extdeg[J(g, k)], W(g, k));
      }
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        alldeg[i] = extdeg[i] + D(g, i);
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i) {
        int64_t v = alldeg[i];
        if (v > 2*ne - v) v = 2*ne - v;
        old_conductance[i] = extdeg[i] / (double)v;
      }

    OMP("omp for schedule(static)") MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);
        double new_conductance;

        int64_t v = alldeg[i] + alldeg[j];
        if (v > 2*ne - v) v = 2*ne - v;
        new_conductance = (extdeg[i] + extdeg[j] - W(g, k)) / (double)v;
        escore[k] = old_conductance[i] + old_conductance[j] - new_conductance;
      }
  }
}

static void
score_cnm (double * restrict escore,
           const struct el g,
           int64_t * restrict alldeg)
{
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  const double weight_sum = g.weight_sum;
  CDECL(g);

  OMP("omp parallel") {
    OMP("omp for schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        alldeg[i] = 0;

    OMP("omp for schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        int64_fetch_add (&alldeg[I(g, k)], W(g, k));
        int64_fetch_add (&alldeg[J(g, k)], W(g, k));
      }

    OMP("omp for schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        alldeg[i] += D(g, i);

    OMP("omp for schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);

        escore[k] = 2 * (W(g, k)/weight_sum - (alldeg[i]/weight_sum) * (alldeg[j]/weight_sum));
      }
  }
}

double mb_nstd = 1.5;

static void
score_mb (double * restrict escore,
          const struct el g,
          int64_t * restrict alldeg)
{
  const int64_t ne = g.ne;
  int64_t final_ne = 0;
  CDECL(g);
  double sum, mean, stddev, thresh;

  score_cnm (escore, g, alldeg);

  /* Treat all edges as the current potential merges, find mean and filter. */
  sum = 0.0;
  OMP("omp parallel") {
    MTA_STREAMS
      OMP("omp for reduction(+:sum) reduction(+:final_ne) schedule(static)")
      for (int64_t k = 0; k < ne; ++k)
        if (escore[k] > 0) {
          sum += escore[k];
          ++final_ne;
        }

    OMP("omp single") {
      mean = sum / final_ne;
      sum = 0.0;
    }
    OMP("omp barrier");
    MTA_STREAMS OMP("omp for reduction(+:sum) schedule(static)")
      for (int64_t k = 0; k < ne; ++k)
        if (escore[k] > 0) {
          double tmp = escore[k]-mean;
          sum += tmp*tmp;
        }

    OMP("omp single") {
      sum /= final_ne - 1;
      stddev = sqrt (sum);
      thresh = mean - mb_nstd * stddev;
    }
    OMP("omp barrier");

    MTA_STREAMS OMP("omp for schedule(static)")
      for (int64_t k = 0; k < ne; ++k)
        if (escore[k] < thresh)
          escore[k] = -0.0;
  }
}

static void
score_drop (double * restrict escore,
            const int64_t mindeg, const int64_t maxdeg,
            const struct el g,
            int64_t * restrict ws)
{
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);

  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        ws[i] = 0;

    OMP("omp for schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);
        OMP("omp atomic") ++ws[i];
        OMP("omp atomic") ++ws[j];
      }

    OMP("omp for schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);
        if (mindeg > 0 && (ws[i] < mindeg || ws[j] < mindeg))
          escore[k] = -0.0;
        if (maxdeg > 0 && (ws[i] > maxdeg || ws[j] > maxdeg))
          escore[k] = -0.0;
      }
  }
}

static void
score_drop_size (double * restrict escore,
                 const int64_t * restrict cmap,
                 const int64_t * restrict csize,
                 const int64_t maxsz,
                 const struct el g)
{
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);

  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t ci = cmap[I(g, k)];
        const intvtx_t cj = cmap[J(g, k)];
        if (csize[ci] > maxsz || csize[cj] > maxsz)
          escore[k] = -0.0;
      }
  }
}

void
eval_cov (const struct el g, const int64_t * restrict c,
          const int64_t orig_nv,
          const int64_t max_in_weight,
          const int64_t nonself_in_weight,
          double * cov_out, double * mirrorcov_out,
          int64_t * ws)
{
  const intvtx_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);
  int64_t tot_weight = g.weight_sum;
  intvtx_t * restrict vtxcount;
  int64_t * restrict border;
  long double cov = 0, mirrorcov = 0;
#if defined(USE_MIRRORCOV)||defined(COMPUTE_MIRRORCOV)
  long double denom;
#endif

  border = ws;
  vtxcount = (intvtx_t*)&border[nv];
#if defined(USE_MIRRORCOV)||defined(COMPUTE_MIRRORCOV)
  denom = orig_nv * ((long double)orig_nv - 1) * max_in_weight - 2*nonself_in_weight;
  //orig_nv * (orig_nv - 1) * max_in_weight - 2*nonself_in_weight;
  assert (denom >= 0);
#endif

  OMP("omp parallel") {
    OMP("omp for schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i) {
        vtxcount[i] = 0;
        border[i] = 0;
      }
    OMP("omp for nowait schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < orig_nv; ++i) {
        const intvtx_t clbl = c[i];
        assert (clbl >= 0 && clbl < nv);
        OMP("omp atomic") ++vtxcount[clbl];
      }
    OMP("omp for nowait schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const intvtx_t i = I(g, k);
        const intvtx_t j = J(g, k);
        const intvtx_t w = W(g, k);
        OMP("omp atomic") border[i] += w;
        OMP("omp atomic") border[j] += w;
      }
    OMP("omp for nowait reduction(+:cov) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i)
        cov += D(g, i) / (long double)tot_weight;
#if defined(USE_MIRRORCOV)||defined(COMPUTE_MIRRORCOV)
    OMP("omp for nowait reduction(+:mirrorcov) schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv; ++i) {
        const long double cnt = vtxcount[i];
        if (cnt)
          mirrorcov += cnt * (orig_nv - cnt) * max_in_weight - 2*border[i];
      }
#endif
  }
#if defined(USE_MIRRORCOV)||defined(COMPUTE_MIRRORCOV)
  mirrorcov /= denom;
#endif
  *cov_out = cov;
  *mirrorcov_out = mirrorcov;
}

static int
cov_not_done (const double covlevel,
              const struct el g, const int64_t * restrict m,
              const intvtx_t orig_nv,
              const int64_t max_in_weight,
              const int64_t nonself_in_weight,
              int64_t * ws)
{
  double cov, mirrorcov;
  eval_cov (g, m, orig_nv, max_in_weight, nonself_in_weight,
            &cov, &mirrorcov, ws);
  if (global_verbose)
    fprintf (stderr, "\tcov %g %g\n", cov, mirrorcov);
#if defined(USE_MIRRORCOV)
  if (cov >= covlevel && mirrorcov >= covlevel) return 0;
#else
  if (cov >= covlevel) return 0;
#endif
  return 1;
}

double global_score_time, global_match_time, global_aftermatch_time,
  global_contract_time, global_other_time;

  /* #define NSTD 1.5 */
#define NSTD 2.5

int64_t
community (int64_t * c, struct el * restrict g /* destructive */,
           const int score_alg, const int match_alg, const int double_self,
           const int decimate,
           const int64_t maxsz, int64_t maxnum,
           const int64_t min_degree, const int64_t max_degree,
           int64_t max_nsteps,
           const double covlevel, const double decrease_factor,
           const int use_hist,
           struct community_hist * restrict hist,
           int64_t * restrict cmap_global, const int64_t nv_global,
           int64_t * ws, size_t wslen,
           void * lockspace_in)
/*
  Will not copy input graph; contraction is in-place.

  Matching workspace:
  non-maximal, or maximal iterative: none
  data flow: nv + 4*ne
  in-between: non-max until larger of remaining g space & ws hold

  Intermediates:
  m for contraction: nv
  score: nv
  conductance ws: 2nv
  contract ws: nv+1 + 2*ne
  gathering weights: nv
*/
{
  const intvtx_t nv_orig = g->nv;
  const int64_t ne_orig = g->ne;

  int64_t * restrict m;
  int64_t * restrict rowstart;
  int64_t * restrict rowend;
  double * restrict score;
  double weight_sum = 0.0;
  int64_t max_in_weight = 0, nonself_in_weight = 0;

  int64_t * ws_inner;

  int64_t csz = -1;
  intvtx_t old_nv;
  int64_t nsteps = 0;
  const int use_cov = covlevel > 0;
  int cov_unsatisfied = 1;
  const int alg_is_matching = (match_alg == ALG_GREEDY_PASS ||
                               match_alg == ALG_GREEDY_MAXIMAL);

  double prev_max_score = 0.0;

  const int verbose = global_verbose;
  double score_time = 0.0, match_time = 0.0, aftermatch_time = 0.0,
    contract_time = 0.0, other_time = 0.0;

  const char *dump_fmt = NULL;
  char dump_name[257];

  struct community_hist h;

#if defined(_OPENMP)
  omp_lock_t * restrict lockspace = (omp_lock_t*)lockspace_in;
#endif

  /* fprintf (stderr, "ne_in %d\n", (int)ne_in); */

  dump_fmt = getenv ("DUMP_FMT");

  assert (wslen > 3*nv_orig + ne_orig);
  m = ws;
  rowstart = &m[nv_orig];
  rowend = &rowstart[nv_orig];
  score = (double*)&rowend[nv_orig];
  ws_inner = (int64_t*)&score[ne_orig];
  wslen -= 3*nv_orig+ne_orig;
  assert (wslen > nv_orig+1 + 2*ne_orig);
  if (max_nsteps < 0) max_nsteps = nv_orig;
  if (maxnum < 2) maxnum = 2;

  if (double_self) {
    struct el gv = *g;
    DECL(gv);
    OMP("omp parallel for") MTA_STREAMS
      for (intvtx_t i = 0; i < nv_orig; ++i)
        D(gv, i) = 2*D(gv, i);
  }
  weight_sum = calc_weight (*g);
  g->weight_sum = weight_sum;
#if !defined(NDEBUG)
  global_gwgt = weight_sum;
  /* fprintf (stderr, "in weight: %ld\n", global_gwgt); */
#endif
  if (use_hist) h = (struct community_hist){{0.0}, 0, 0};

  tic ();
  OMP("omp parallel") {
    int64_t local_max_weight = 0;
    struct el gv = *g;
    DECL(gv);
    OMP("omp for reduction(+:nonself_in_weight) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne_orig; ++k) {
        assert (I(gv, k) != J(gv, k));
        canonical_order_edge (&I(gv, k), &J(gv, k));
        if (W(gv, k) > local_max_weight) local_max_weight = W(gv, k);
        nonself_in_weight += W(gv, k);
      }
    OMP("omp critical") {
      if (local_max_weight > max_in_weight) max_in_weight = local_max_weight;
    }
    rough_bucket_sort_el (nv_orig, ne_orig, g->el,
                          ws_inner, (intvtx_t*)&ws_inner[nv_orig+1]);
    rough_bucket_copy_back (nv_orig, &nsteps,
                            g->el, rowstart, rowend,
                            ws_inner, 1+ws_inner, (intvtx_t*)&ws_inner[nv_orig+1]);
    assert (nsteps == g->ne);

    /* Initialize with all vertices separate */
    OMP("omp for nowait schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv_orig; ++i)
        c[i] = i;
    OMP("omp for schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < gv.ne; ++k)
        score[k] = -2962.5;
  }
  if (use_cov)
    cov_unsatisfied = cov_not_done (covlevel, *g, c, nv_orig,
                                    max_in_weight, nonself_in_weight,
                                    ws_inner);

  if (cmap_global) {
    OMP("omp parallel") {
      int64_t tcsz = -1;
      OMP("omp for")
        for (int64_t k = 0; k < nv_orig; ++k)
          ws_inner[k] = 0;
      OMP("omp for")
        for (int64_t k = 0; k < nv_global; ++k)
          OMP("omp atomic") ++ws_inner[cmap_global[k]];
      OMP("omp for")
        for (int64_t k = 0; k < nv_orig; ++k)
          if (ws_inner[k] > tcsz) tcsz = ws_inner[k];
      OMP("omp critical")
        if (csz < tcsz) csz = tcsz;
    }
  } else {
      csz = 1;
  }
  
  other_time += toc ();

  nsteps = 0;
  while (nsteps < max_nsteps && csz <= maxsz && g->nv >= maxnum &&
         cov_unsatisfied) {
    intvtx_t new_nv = -1;
#if !defined(NDEBUG)
    int64_t iterwgt, contractedwgt;
    iterwgt = calc_weight (*g);
    assert (iterwgt == global_gwgt);
#endif


    /* score */
    if (verbose) fprintf (stderr, "\nstep %ld\n\tscore ...", (long)nsteps+1);
    tic ();
    switch (score_alg) {
    case ALG_HEAVIEST_EDGE:
      score_heaviest_edge (score, *g);
      break;
    case ALG_CONDUCTANCE:
      score_conductance (score, *g, ws_inner);
      break;
    case ALG_CNM:
      score_cnm (score, *g, ws_inner);
      break;
    case ALG_MB:
      score_mb (score, *g, ws_inner);
      break;
    default:
      fprintf (stderr, "ugh\n");
      abort ();
    }
    if (min_degree > 0 || max_degree > 0)
      score_drop (score, min_degree, max_degree, *g, ws_inner);
    score_time += toc ();
    double max_score = -HUGE_VAL;
    OMP("omp parallel") {
      double tmax_score = -HUGE_VAL;
      OMP("omp for nowait schedule(static)") MTA_STREAMS
        for (int64_t k = 0; k < g->ne; ++k)
          if (score[k] > tmax_score) tmax_score = score[k];
      OMP("omp critical")
        if (tmax_score > max_score) max_score = tmax_score;
    }
    if (verbose) {
      fprintf (stderr, "done : %ld, max score %g\n", nsteps, max_score);
    }

    if (decrease_factor > 0 && max_score < prev_max_score / decrease_factor) {
      if (verbose)
        fprintf (stderr, "Maximum score dropped from %g to %g (factor %g).  Stopping.\n",
                 prev_max_score, max_score, max_score / prev_max_score);
      break;
    }
    prev_max_score = max_score;

    if (use_hist) {
      if (h.nhist) {
        double mean = 0, sd = 0;
        for (int k = 0; k < h.nhist; ++k)
          mean += h.hist[(h.curhist + k) % NHIST];
        mean /= h.nhist;
        for (int k = 0; k < h.nhist; ++k) {
          double d = h.hist[(h.curhist + k) % NHIST] - mean;
          sd += d * d;
        }
        sd = sqrt (sd / (h.nhist-1));
        /* if (verbose) */
        /*   fprintf (stderr, "Maximum score %g, %g - 1.5 * %g = %g.\n", max_score, mean, sd, mean-1.5*sd); */
        if (max_score < mean - NSTD * sd) {
          if (verbose)
            fprintf (stderr, "Maximum score %g dropped below %g - %g * %g = %g.  Stopping.\n",
                     max_score, mean, NSTD, sd, mean-NSTD*sd);
          break;
        }
        /* if (max_score < NSTD * sd) { */
        /*   if (verbose) */
        /*     fprintf (stderr, "Maximum score %g dropped below %g * %g = %g.  Stopping.\n", */
        /*       max_score, NSTD, sd, NSTD*sd); */
        /*   break; */
        /* } */
      }
      h.hist[h.curhist] = max_score;
      h.curhist = (h.curhist+1)%NHIST;
      if (h.nhist < NHIST) ++h.nhist;
    }

    /* match */
    if (verbose) fprintf (stderr, "\tmatch ...");
    tic ();
    old_nv = g->nv;
    switch (match_alg) {
    case ALG_GREEDY_PASS:
      OMP("omp parallel for schedule(static)") MTA_STREAMS
        for (intvtx_t i = 0; i < old_nv; ++i) {
          m[i] = -1;
          ws_inner[i] = i;
        }
      sliced_non_maximal_pass (*g, rowstart, rowend, old_nv, (intvtx_t*)ws_inner,
                               m, score,
                               &ws_inner[old_nv], (intvtx_t*)&ws_inner[2*old_nv]
                               OMP_LOCKSPACE_PARAM);
      break;
    case ALG_GREEDY_MAXIMAL:
      maximal_match_iter (*g, rowstart, rowend, m, score, ws_inner
                          OMP_LOCKSPACE_PARAM);
      break;
    case ALG_TREE:
      simple_tree_cover (*g, rowstart, rowend, m, score, ws_inner
                         OMP_LOCKSPACE_PARAM);
      break;
    default:
      fprintf (stderr, "ugh-match\n");
      abort ();
    };
    match_time += toc ();
    if (alg_is_matching) {
      if (decimate)
        decimate_matching (*g, rowstart, rowend, m, score);
      if (verbose) {
        const struct el cg = *g;
        CDECL(cg);
        double match_score = 0;
        intvtx_t nmatch = 0;
        OMP("omp parallel for reduction(+:match_score) reduction(+:nmatch)")
          MTA_STREAMS
          for (intvtx_t i = 0; i < old_nv; ++i) {
            if (m[i] >= 0) {
              if (i == I(cg, m[i])) {
                match_score += score[m[i]];
                ++nmatch;
              }
            }
          }
        fprintf (stderr, "done (nmatch %ld, sum score %g)\n", (long)nmatch,
                 match_score);
      }
      new_nv = convert_el_match_to_relabel (*g, m, ws_inner);
    } else {
      if (verbose) {
        const struct el cg = *g;
        CDECL(cg);
        double edge_sum_score = 0;
        intvtx_t nedge = 0;
        OMP("omp parallel for reduction(+:edge_sum_score) reduction(+:nedge)")
          MTA_STREAMS
          for (intvtx_t ki = 0; ki < old_nv; ++ki) {
            if (m[ki] >= 0) {
              intvtx_t i, j;
              LOAD_ORDERED (cg, m[ki], i, j);
              if (m[i] != m[j] || i < j) {
                edge_sum_score += score[m[i]];
                ++nedge;
              }
            }
          }
        fprintf (stderr, "done (nedge %ld, sum score %g)\n", (long)nedge,
                 edge_sum_score);
      }
      new_nv = convert_el_tree_to_relabel (*g, m, ws_inner);
    }
    aftermatch_time += toc ();

    /* contract */
    if (verbose) fprintf (stderr, "\tcontract ...");
    tic ();
    contract (g, new_nv, m, rowstart, rowend, ws_inner);
    OMP("omp parallel for") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < nv_orig; ++i)
        if (c[i] >= 0) c[i] = m[c[i]];
    contract_time += toc ();
    if (verbose) fprintf (stderr, "done %ld\n", (long)g->nv);
#if !defined(NDEBUG)
    assert (g->nv == new_nv);
    contractedwgt = calc_weight (*g);
    assert (contractedwgt == global_gwgt);
#endif

#if !defined(NDEBUG)
    OMP("omp parallel for") MTA("mta assert parallel") MTA_STREAMS
      for (int64_t k = 0; k < g->ne; ++k) {
        assert (Iold(*g, k) != Jold(*g, k));
      }
#endif

    tic ();

    if (old_nv == g->nv) {
      if (verbose)
        fprintf (stderr, "No vertices contracted.  Stopping.\n");
      break;
    }
    if (!g->ne) {
      if (verbose)
        fprintf (stderr, "No edges remain.  Stopping.\n");
      break;
    }
    if (use_cov)
      cov_unsatisfied = cov_not_done (covlevel, *g, c, nv_orig,
                                      max_in_weight, nonself_in_weight,
                                      ws_inner);

    csz = 0;
    if (!cmap_global) {
      OMP("omp parallel") {
        int64_t local_csz = 0;
        OMP("omp for") MTA_STREAMS
          for (intvtx_t i = 0; i < g->nv; ++i)
            ws_inner[i] = 0;
        OMP("omp for") MTA_NODEP MTA_STREAMS
          for (intvtx_t i = 0; i < nv_orig; ++i)
            if (c[i] >= 0)
              int64_fetch_add (&ws_inner[c[i]], 1);
        OMP("omp for") MTA_STREAMS
          for (intvtx_t i = 0; i < g->nv; ++i)
            if (ws_inner[i] > local_csz) local_csz = ws_inner[i];
        OMP("omp critical") {
          if (local_csz > csz) csz = local_csz;
        }
      }
    } else {
      OMP("omp parallel") {
        int64_t local_csz = 0;
        /* First remap cmap_global */
        OMP("omp for")
          for (int64_t k = 0; k < nv_global; ++k)
            cmap_global[k] = c[cmap_global[k]];
        /* Now count. */
        OMP("omp for") MTA_STREAMS
          for (intvtx_t i = 0; i < g->nv; ++i)
            ws_inner[i] = 0;
        OMP("omp for") MTA_NODEP MTA_STREAMS
          for (intvtx_t i = 0; i < nv_global; ++i)
            if (cmap_global[i] >= 0)
              int64_fetch_add (&ws_inner[cmap_global[i]], 1);
        OMP("omp for") MTA_STREAMS
          for (intvtx_t i = 0; i < g->nv; ++i)
            if (ws_inner[i] > local_csz) local_csz = ws_inner[i];
        OMP("omp critical") {
          if (local_csz > csz) csz = local_csz;
        }
      }
    }
    ++nsteps;
    other_time += toc ();
  }

  *hist = h;
  global_score_time = score_time;
  global_match_time = match_time;
  global_aftermatch_time = aftermatch_time;
  global_contract_time = contract_time;
  global_other_time = other_time;
  return nsteps;
}

int64_t
update_community (int64_t * restrict cmap_global, const int64_t nv_global,
                  int64_t * restrict csize,
                  int64_t * restrict max_csize,
		  int64_t * restrict n_nonsingletons_out,

                  struct el * restrict g /* destructive */,
                  const int score_alg, const int match_alg, const int double_self,
                  const int decimate,
                  const int64_t maxsz, int64_t maxnum,
                  const int64_t min_degree, const int64_t max_degree,
                  int64_t max_nsteps,
                  const double covlevel, const double decrease_factor,
                  const int use_hist,
                  struct community_hist * restrict hist,

                  int64_t * ws, size_t wslen,
                  void * lockspace_in)
/*
  Will not copy input graph; contraction is in-place.

  Matching workspace:
  non-maximal, or maximal iterative: none
  data flow: nv + 4*ne
  in-between: non-max until larger of remaining g space & ws hold

  Intermediates:
  c: nv
  m for contraction: nv
  score: nv
  conductance ws: 2nv
  contract ws: nv+1 + 2*ne
  gathering weights: nv
*/
{
  const intvtx_t nv_orig = g->nv;
  const int64_t ne_orig = g->ne;

  int64_t * restrict c;
  int64_t * restrict m;
  int64_t * restrict rowstart;
  int64_t * restrict rowend;
  double * restrict score;
  double weight_sum = 0.0;
  int64_t max_in_weight = 0, nonself_in_weight = 0;

  int64_t * ws_inner;

  int64_t csz = -1;
  int64_t n_nonsingletons = 0;
  intvtx_t old_nv;
  int64_t nsteps = 0;
  const int use_cov = covlevel > 0;
  int cov_unsatisfied = 1;
  const int alg_is_matching = (match_alg == ALG_GREEDY_PASS ||
                               match_alg == ALG_GREEDY_MAXIMAL);

  double prev_max_score = 0.0;

  const int verbose = global_verbose;
  double score_time = 0.0, match_time = 0.0, aftermatch_time = 0.0,
    contract_time = 0.0, other_time = 0.0;

  const char *dump_fmt = NULL;
  char dump_name[257];

  struct community_hist h;

#if defined(_OPENMP)
  omp_lock_t * restrict lockspace = (omp_lock_t*)lockspace_in;
#endif

  /* fprintf (stderr, "ne_in %d\n", (int)ne_in); */

  dump_fmt = getenv ("DUMP_FMT");

  assert (wslen > 4*nv_orig + ne_orig);
  c = ws;
  m = &c[nv_orig];
  rowstart = &m[nv_orig];
  rowend = &rowstart[nv_orig];
  score = (double*)&rowend[nv_orig];
  ws_inner = (int64_t*)&score[ne_orig];
  wslen -= 4*nv_orig+ne_orig;
  assert (wslen > nv_orig + ne_orig);
  if (max_nsteps < 0) max_nsteps = nv_orig;
  if (maxnum < 2) maxnum = 2;

  if (use_hist) h = *hist;

  weight_sum = calc_weight (*g);
  g->weight_sum = weight_sum;
#if !defined(NDEBUG)
  global_gwgt = weight_sum;
  /* fprintf (stderr, "in weight: %ld\n", global_gwgt); */
#endif

  tic ();
  OMP("omp parallel") {
    int64_t local_max_weight = 0;
    struct el gv = *g;
    DECL(gv);
    OMP("omp for reduction(+:nonself_in_weight) schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < ne_orig; ++k) {
        assert (I(gv, k) != J(gv, k));
        canonical_order_edge (&I(gv, k), &J(gv, k));
        if (W(gv, k) > local_max_weight) local_max_weight = W(gv, k);
        nonself_in_weight += W(gv, k);
      }
    OMP("omp critical") {
      if (local_max_weight > max_in_weight) max_in_weight = local_max_weight;
    }
    rough_bucket_sort_el (nv_orig, ne_orig, g->el,
                          ws_inner, (intvtx_t*)&ws_inner[nv_orig+1]);
    rough_bucket_copy_back (nv_orig, &nsteps,
                            g->el, rowstart, rowend,
                            ws_inner, 1+ws_inner, (intvtx_t*)&ws_inner[nv_orig+1]);
    assert (nsteps == g->ne);

    /* Initialize with all vertices separate */
    OMP("omp for nowait schedule(static)") MTA_STREAMS
      for (intvtx_t i = 0; i < nv_orig; ++i)
        c[i] = i;
    OMP("omp for schedule(static)") MTA_STREAMS
      for (int64_t k = 0; k < gv.ne; ++k)
        score[k] = -2962.5;
  }
  if (use_cov)
    cov_unsatisfied = cov_not_done (covlevel, *g, c, nv_orig,
                                    max_in_weight, nonself_in_weight,
                                    ws_inner);

  OMP("omp parallel") {
    int64_t tcsz = -1;
    OMP("omp for")
      for (int64_t k = 0; k < nv_orig; ++k)
        if (csize[k] > tcsz) tcsz = csize[k];
      OMP("omp critical")
        if (csz < tcsz) csz = tcsz;
    }
  
  other_time += toc ();

  nsteps = 0;
  while (nsteps < max_nsteps && csz <= maxsz && g->nv >= maxnum &&
         cov_unsatisfied) {
    intvtx_t old_nc = g->nv;
    intvtx_t new_nv = -1;
#if !defined(NDEBUG)
    int64_t iterwgt, contractedwgt;
    iterwgt = calc_weight (*g);
    assert (iterwgt == global_gwgt);
#endif


    /* score */
    if (verbose) fprintf (stderr, "\nstep %ld\n\tscore ...", (long)nsteps+1);
    tic ();
    switch (score_alg) {
    case ALG_HEAVIEST_EDGE:
      score_heaviest_edge (score, *g);
      break;
    case ALG_CONDUCTANCE:
      score_conductance (score, *g, ws_inner);
      break;
    case ALG_CNM:
      score_cnm (score, *g, ws_inner);
      break;
    case ALG_MB:
      score_mb (score, *g, ws_inner);
      break;
    default:
      fprintf (stderr, "ugh\n");
      abort ();
    }
    if (min_degree > 0 || max_degree > 0)
      score_drop (score, min_degree, max_degree, *g, ws_inner);
    if (maxsz < INT64_MAX)
      score_drop_size (score, c, csize, maxsz, *g);
    score_time += toc ();
    double max_score = -HUGE_VAL;
    OMP("omp parallel") {
      double tmax_score = -HUGE_VAL;
      OMP("omp for nowait schedule(static)") MTA_STREAMS
        for (int64_t k = 0; k < g->ne; ++k)
          if (score[k] > tmax_score) tmax_score = score[k];
      OMP("omp critical")
        if (tmax_score > max_score) max_score = tmax_score;
    }
    if (verbose) {
      fprintf (stderr, "done : %ld, max score %g\n", nsteps, max_score);
    }

    if (decrease_factor > 0 && max_score < prev_max_score / decrease_factor) {
      if (verbose)
        fprintf (stderr, "Maximum score dropped from %g to %g (factor %g).  Stopping.\n",
                 prev_max_score, max_score, max_score / prev_max_score);
      break;
    }
    prev_max_score = max_score;

    if (use_hist) {
      if (h.nhist) {
        double mean = 0, sd = 0;
        for (int k = 0; k < h.nhist; ++k)
          mean += h.hist[(h.curhist + k) % NHIST];
        mean /= h.nhist;
        for (int k = 0; k < h.nhist; ++k) {
          double d = h.hist[(h.curhist + k) % NHIST] - mean;
          sd += d * d;
        }
        sd = sqrt (sd / (h.nhist-1));
        /* if (verbose) */
        /*   fprintf (stderr, "Maximum score %g, %g - 1.5 * %g = %g.\n", max_score, mean, sd, mean-1.5*sd); */
        if (max_score < mean - NSTD * sd) {
          if (verbose)
            fprintf (stderr, "Maximum score %g dropped below %g - %g * %g = %g.  Stopping.\n",
                     max_score, mean, NSTD, sd, mean-NSTD*sd);
          break;
        }
        /* if (max_score < NSTD * sd) { */
        /*   if (verbose) */
        /*     fprintf (stderr, "Maximum score %g dropped below %g * %g = %g.  Stopping.\n", */
        /*       max_score, NSTD, sd, NSTD*sd); */
        /*   break; */
        /* } */
      }
      h.hist[h.curhist] = max_score;
      h.curhist = (h.curhist+1)%NHIST;
      if (h.nhist < NHIST) ++h.nhist;
    }

    /* match */
    if (verbose) fprintf (stderr, "\tmatch ...");
    tic ();
    old_nv = g->nv;
    switch (match_alg) {
    case ALG_GREEDY_PASS:
      OMP("omp parallel for schedule(static)") MTA_STREAMS
        for (intvtx_t i = 0; i < old_nv; ++i) {
          m[i] = -1;
          ws_inner[i] = i;
        }
      sliced_non_maximal_pass (*g, rowstart, rowend, old_nv, (intvtx_t*)ws_inner,
                               m, score,
                               &ws_inner[old_nv], (intvtx_t*)&ws_inner[2*old_nv]
                               OMP_LOCKSPACE_PARAM);
      break;
    case ALG_GREEDY_MAXIMAL:
      maximal_match_iter (*g, rowstart, rowend, m, score, ws_inner
                          OMP_LOCKSPACE_PARAM);
      break;
    case ALG_TREE:
      simple_tree_cover (*g, rowstart, rowend, m, score, ws_inner
                         OMP_LOCKSPACE_PARAM);
      break;
    default:
      fprintf (stderr, "ugh-match\n");
      abort ();
    };
    match_time += toc ();
    if (alg_is_matching) {
      if (decimate)
        decimate_matching (*g, rowstart, rowend, m, score);
      if (verbose) {
        const struct el cg = *g;
        CDECL(cg);
        double match_score = 0;
        intvtx_t nmatch = 0;
        OMP("omp parallel for reduction(+:match_score) reduction(+:nmatch)")
          MTA_STREAMS
          for (intvtx_t i = 0; i < old_nv; ++i) {
            if (m[i] >= 0) {
              if (i == I(cg, m[i])) {
                match_score += score[m[i]];
                ++nmatch;
              }
            }
          }
        fprintf (stderr, "done (nmatch %ld, sum score %g)\n", (long)nmatch,
                 match_score);
      }
      new_nv = convert_el_match_to_relabel (*g, m, ws_inner);
    } else {
      if (verbose) {
        const struct el cg = *g;
        CDECL(cg);
        double edge_sum_score = 0;
        intvtx_t nedge = 0;
        OMP("omp parallel for reduction(+:edge_sum_score) reduction(+:nedge)")
          MTA_STREAMS
          for (intvtx_t ki = 0; ki < old_nv; ++ki) {
            if (m[ki] >= 0) {
              intvtx_t i, j;
              LOAD_ORDERED (cg, m[ki], i, j);
              if (m[i] != m[j] || i < j) {
                edge_sum_score += score[m[i]];
                ++nedge;
              }
            }
          }
        fprintf (stderr, "done (nedge %ld, sum score %g)\n", (long)nedge,
                 edge_sum_score);
      }
      new_nv = convert_el_tree_to_relabel (*g, m, ws_inner);
    }
    aftermatch_time += toc ();

    /* contract */
    if (verbose) fprintf (stderr, "\tcontract ...");
    tic ();
    contract (g, new_nv, m, rowstart, rowend, ws_inner);
    OMP("omp parallel for") MTA_NODEP MTA_STREAMS
      for (intvtx_t i = 0; i < nv_orig; ++i)
        if (c[i] >= 0) c[i] = m[c[i]];
    contract_time += toc ();
    if (verbose) fprintf (stderr, "done %ld\n", (long)g->nv);
#if !defined(NDEBUG)
    contractedwgt = calc_weight (*g);
    assert (contractedwgt == global_gwgt);
    OMP("omp parallel") {
      /* Check that all the new communities are used... */
      OMP("omp for")
        for (int64_t k = 0; k < new_nv; ++k)
          ws_inner[k] = 0;
      OMP("omp for")
        for (int64_t k = 0; k < nv_orig; ++k)
          OMP("omp atomic") ++ws_inner[c[k]];
      OMP("omp for")
        for (int64_t k = 0; k < new_nv; ++k) {
          assert (ws_inner[k] > 0);
          assert (ws_inner[k] <= nv_global);
        }
    }
#endif

#if !defined(NDEBUG)
    OMP("omp parallel for") MTA("mta assert parallel") MTA_STREAMS
      for (int64_t k = 0; k < g->ne; ++k) {
        assert (Iold(*g, k) != Jold(*g, k));
      }
#endif

    tic ();

    if (old_nv == g->nv) {
      if (verbose)
        fprintf (stderr, "No vertices contracted.  Stopping.\n");
      break;
    }
    if (!g->ne) {
      if (verbose)
        fprintf (stderr, "No edges remain.  Stopping.\n");
      break;
    }
    if (use_cov)
      cov_unsatisfied = cov_not_done (covlevel, *g, c, nv_orig,
                                      max_in_weight, nonself_in_weight,
                                      ws_inner);

    assert (g->nv == new_nv);
    csz = 0;
    n_nonsingletons = 0;
    int64_t totsz = 0;
    OMP("omp parallel") {
      int64_t local_csz = 0;
      OMP("omp for") MTA_STREAMS
        for (intvtx_t i = 0; i < new_nv; ++i)
          ws_inner[i] = 0;
      OMP("omp for")
        for (int64_t k = 0; k < old_nc; ++k) {
          intvtx_t newc = m[k];
          OMP("omp atomic") ws_inner[newc] += csize[k];
        }
      OMP("omp for")
        for (int64_t k = 0; k < nv_global; ++k) {
          intvtx_t oldc = cmap_global[k];
          intvtx_t newc = m[oldc];
          cmap_global[k] = newc;
          /* int64_fetch_add (&ws_inner[newc], 1); */
        }
      OMP("omp for reduction(+: totsz, n_nonsingletons)") MTA_STREAMS
        for (intvtx_t i = 0; i < new_nv; ++i) {
	  const intvtx_t z = ws_inner[i];
          if (z > local_csz) local_csz = z;
          csize[i] = z;
	  if (z > 1) ++n_nonsingletons;
          totsz += z;
        }
      OMP("omp critical") {
        if (local_csz > csz) csz = local_csz;
      }
      assert (totsz == nv_global);
    }

    ++nsteps;
    other_time += toc ();
  }

  *max_csize = csz;
  *n_nonsingletons_out = n_nonsingletons;
  *hist = h;
  global_score_time = score_time;
  global_match_time = match_time;
  global_aftermatch_time = aftermatch_time;
  global_contract_time = contract_time;
  global_other_time = other_time;
  return nsteps;
}

double
eval_conductance_cgraph (const struct el g, int64_t * ws)
{
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);
  int64_t * restrict vol = ws;
  int64_t * restrict ncut = &ws[nv];
  int64_t totv = 0;
  double cond = 0;

  OMP("omp parallel") {
    OMP("omp for reduction(+: totv) schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t i = 0; i < nv; ++i) {
        vol[i] = D(g, i);
        totv += vol[i];
        ncut[i] = 0;
      }

    OMP("omp for reduction(+: totv) schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const int64_t i = I(g, k);
        const int64_t j = J(g, k);
        const int64_t w = W(g, k);

        totv += 2*w;
        vol[i] += w;
        vol[j] += w;
        ncut[i] += w;
        ncut[j] += w;
      }

    OMP("omp for reduction(+:cond) schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t i = 0; i < nv; ++i) {
        const double denom = 2*vol[i] > totv? totv - vol[i] : vol[i];
        const double numer = ncut[i];
        double comm_cond = 0.0;
        if (denom)
          comm_cond = numer / denom;
        else if (numer)
          comm_cond = HUGE_VAL;
        cond += comm_cond;
      }
  }
  return cond / nv;
}

double
eval_modularity_cgraph (const struct el g, int64_t * ws)
{
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;
  CDECL(g);
  int64_t totw = 0;
  double out = 0.0;

  int64_t * restrict Lsplus = ws;

  OMP("omp parallel") {
    OMP("omp for reduction(+: totw) schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t i = 0; i < nv; ++i) {
        Lsplus[i] = 4*D(g, i);
        totw += 2*D(g, i);
      }

    OMP("omp for reduction(+: totw) schedule(static)")
      MTA_NODEP
      MTA_STREAMS
      for (int64_t k = 0; k < ne; ++k) {
        const int64_t i = I(g, k);
        const int64_t j = J(g, k);
        const int64_t w = W(g, k);

        int64_fetch_add (&Lsplus[i], 2*w);
        int64_fetch_add (&Lsplus[j], 2*w);
        totw += 2*w;
      }

    OMP("omp for reduction(+: out) schedule(static)")
      MTA_STREAMS
      for (int64_t i = 0; i < nv; ++i) {
        /* if (i < 20) */
        /*	fprintf (stderr, "%ld: %g %g %g\n", (long)i, */
        /*     (double)(2*D(g,i)), (double)totw, */
        /*     (double)Lsplus[i]); */
        out += (2*D(g, i) - (Lsplus[i]/2.0)*(Lsplus[i]/2.0)/totw) / totw;
      }
  }

  return out;
}





static int64_t
contract_self_el (int64_t NE, intvtx_t * restrict el /* 3 x oldNE */,
                  intvtx_t nv,
                  intvtx_t * restrict d /* oldNV */,
                  int64_t * restrict rowstart /* newNV */,
                  int64_t * restrict rowend /* newNV */,
                  int64_t * restrict ws /* newNV+1 + 2*oldNE */)
{
  int64_t n_new_edges = 0;

#if !defined(NDEBUG)
  int64_t w_in = 0, w_out = 0;
#endif

  int64_t * restrict count = ws;
  intvtx_t * restrict tmpcopy = (intvtx_t*)&count[nv+1];

  /* fprintf (stderr, "from %ld => %ld\n", old_nv, nv); */

  OMP("omp parallel") 
  {
#if !defined(NDEBUG)
    OMP("omp for reduction(+:w_in) schedule(static)")
      for (intvtx_t i = 0; i < nv; ++i)
        w_in += d[i];
    OMP("omp for reduction(+:w_in) schedule(static)")
      for (int64_t k = 0; k < NE; ++k)
        w_in += Wel(el, k);
    OMP("omp barrier")
#endif

#if !defined(NDEBUG)
    int64_t lw_out = all_calc_weight_base_flat (nv, NE, el, d);
    OMP("omp master")
      w_out = lw_out;
    OMP("omp barrier");
    OMP("omp single")
      if (w_in != w_out)
        fprintf (stderr, "%ld %ld %ld\n", (long)w_in, (long)w_out, (long)global_gwgt);
    OMP("omp barrier");
    OMP("omp master")
      assert (w_out == w_in);
    OMP("omp barrier");
    OMP("omp single") w_out = 0;
#endif

    OMP("omp for schedule(static)") MTA_NODEP
      for (int64_t k = 0; k < NE; ++k)
        canonical_order_edge (&Iel(el, k), &Jel(el, k));

    rough_bucket_sort_el (nv, NE, el, count, tmpcopy);
    assert (NE == count[nv]);

#if !defined(NDEBUG)
    OMP("omp master") {
      w_out = 0;
    }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)") 
      for (intvtx_t i = 0; i < nv; ++i)
        w_out += d[i];
    OMP("omp for reduction(+:w_out) schedule(static)")
      for (intvtx_t i = 0; i < nv; ++i) {
        for (int64_t k = count[i]; k < count[1+i]; ++k) {
          w_out += tmpcopy[1+2*k];
        }
      }
    OMP("omp master") {
      if (w_in != w_out) {
        fprintf (stderr, "%d: w_in %ld w_out %ld\n",
#if defined(_OPENMP)
                 omp_get_thread_num (),
#else
                 1,
#endif
                 w_in, w_out);
      }
      assert (w_in == w_out);

    }
#endif

    /* int64_t dumped_row = 0; */
    /* Sort then collapse within each row. */
    OMP("omp for schedule(guided)") MTA("mta assert parallel")
      for (intvtx_t new_i = 0; new_i < nv; ++new_i) {
        const int64_t new_i_end = count[new_i+1];
        int64_t k, kcur = count[new_i];
#if !defined(NDEBUG)
        int64_t diagsum = 0, row_w_in = 0, row_w_out = 0;
        for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2)
          row_w_in += tmpcopy[1+2*k2];
#endif
        //if (dumped_row < 2 && new_i_end - count[new_i] > 3) {
        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld A:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) */
        /*    fprintf (stderr, " (%ld; %ld)", tmpcopy[2*k2], tmpcopy[1+2*k2]); */
        /*	fprintf (stderr, "\n"); */
        /* } */

        introsort_iki (&tmpcopy[2*count[new_i]], new_i_end - count[new_i]);
#if !defined(NDEBUG)
        /* fprintf (stderr, " %ld", tmpcopy[2*count[new_i]]); */
        for (int64_t k2 = count[new_i]+1; k2 < new_i_end; ++k2) {
          /* fprintf (stderr, " %ld", tmpcopy[2*k2]); */
          assert (tmpcopy[2*k2] >= tmpcopy[2*(k2-1)]);
        }
        /* fprintf (stderr, "\n"); */
#endif

        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld B:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) */
        /*    fprintf (stderr, " (%ld; %ld)", tmpcopy[2*k2], tmpcopy[1+2*k2]); */
        /*	fprintf (stderr, "\n"); */
        /* } */

        for (k = count[new_i]; k < new_i_end; ++k) {
          if (new_i == tmpcopy[2*k]) {
#if !defined(NDEBUG)
            diagsum += tmpcopy[1+2*k];
#endif
            d[new_i] += tmpcopy[1+2*k];
          } else
            break;
        }

        kcur = count[new_i];

        if (k == new_i_end) {
          /* Left with an empty row. */
          rowend[new_i] = kcur;
        } else {
          if (k != kcur) {
            tmpcopy[2*kcur] = tmpcopy[2*k];
            tmpcopy[1+2*kcur] = tmpcopy[1+2*k];
          }
          for (++k; k < new_i_end; ++k) {
            if (new_i == tmpcopy[2*k]) {
#if !defined(NDEBUG)
              diagsum += tmpcopy[1+2*k];
#endif
              d[new_i] += tmpcopy[1+2*k];
            } else if (tmpcopy[2*k] != tmpcopy[2*kcur]) {
              ++kcur;
              if (kcur != k) {
                tmpcopy[2*kcur] = tmpcopy[2*k];
                tmpcopy[1+2*kcur] = tmpcopy[1+2*k];
              }
            } else
              tmpcopy[1+2*kcur] += tmpcopy[1+2*k];
          }
          rowend[new_i] = kcur+1;
        }

        /* if (new_i < 5) { */
        /*	fprintf (stderr, "row %ld C:", new_i); */
        /*	for (int64_t k2 = count[new_i]; k2 < new_i_end; ++k2) { */
        /*    char *sep = " "; */
        /*    char *sep2 = ""; */
        /*    if (k2 == kcur) sep2 = sep = "|"; */
        /*    fprintf (stderr, "%s(%ld; %ld)%s", sep, tmpcopy[2*k2], tmpcopy[1+2*k2], sep2); */
        /*	} */
        /*	fprintf (stderr, "\n"); */
        /*	++dumped_row; */
        /* } */

#if !defined(NDEBUG)
        row_w_out = diagsum;
        for (int64_t k2 = count[new_i]; k2 < rowend[new_i]; ++k2)
          row_w_out += tmpcopy[1+2*k2];
        if (row_w_in != row_w_out)
          fprintf (stderr, "row %ld mismatch %ld => %ld   %ld\n", (long)new_i,
                   row_w_in, row_w_out, diagsum);
#endif
      }

#if !defined(NDEBUG)
    OMP("omp master") w_out = 0;
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)")
      for (intvtx_t i = 0; i < nv; ++i) {
        w_out += d[i];
      }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)")
      for (intvtx_t i = 0; i < nv; ++i) {
        for (int64_t k = count[i]; k < rowend[i]; ++k)
          w_out += tmpcopy[1+2*k];
      }
    OMP("omp master")
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    assert (w_in == w_out);
#endif

    rough_bucket_copy_back (nv, &n_new_edges, el,
                            rowstart, rowend,
                            count, rowend, tmpcopy);

#if !defined(NDEBUG)
    //w_out = all_calc_weight_base_flat (nv, n_new_edges, el, d);
    OMP("omp master") w_out = 0;
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)")
      for (intvtx_t i = 0; i < nv; ++i) {
        w_out += d[i];
      }
    OMP("omp barrier");
    OMP("omp for reduction(+:w_out) schedule(static)")
      for (int64_t k = 0; k < n_new_edges; ++k) {
        assert (Iel(el, k) != Jel(el, k));
        w_out += Wel(el, k);
      }
    OMP("omp master") {
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    }
    assert (w_in == w_out);
#endif

#if !defined(NDEBUG)
    w_out = all_calc_weight_base (nv, n_new_edges, el, d,
                                  rowstart, rowend);
    OMP("omp master") {
      if (w_in != w_out) {
        fprintf (stderr, "w_in %ld w_out %ld\n", w_in, w_out);
      }
    }
    OMP("omp barrier");
    assert (w_in == w_out);
#endif
  }

  assert (n_new_edges <= NE);
  return n_new_edges;
}

void
contract_self (struct el * restrict g,
               int64_t * restrict ws_in /* 3*NV+1 + 2*NE */)
{
  const intvtx_t nv = g->nv;
  const int64_t ne = g->ne;
  int64_t new_ne;
  int64_t * rowstart = ws_in;
  int64_t * rowend = &rowstart[nv];
  int64_t * ws = &rowend[nv];
#if !defined(NDEBUG)
  int64_t w_in, w_out;
  w_in = calc_weight (*g);
#endif

#if !defined(NDEBUG)
  {
    struct el el = *g;
    CDECL(el);
    for (int64_t k = 0; k < el.ne; ++k) {
      assert (I(el, k) < el.nv);
      assert (J(el, k) < el.nv);
      assert (I(el, k) >= 0);
      assert (J(el, k) >= 0);
    }
  }
#endif

  new_ne = contract_self_el (ne, g->el, nv, g->d,
                             rowstart, rowend, ws);
  g->ne = new_ne;
#if !defined(NDEBUG)
  w_out = calc_weight (*g);
  assert (w_in == w_out);
#endif
}
