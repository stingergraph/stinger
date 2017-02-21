#define _FILE_OFFSET_BITS 64
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <errno.h>
#include <assert.h>

#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>

#include "compat.h"
#include "stinger_core/xmalloc.h"
#include "graph-el.h"

#include "bsutil.h"

int use_file_allocation = 0;

static int io_ready = 0;

static void
io_init (void)
{
  io_ready = 1;
}

static ssize_t
xread(int fd, void *databuf, size_t len)
{
  unsigned char *buf = (unsigned char*)databuf;
  size_t totread = 0, toread = len;
  ssize_t lenread;
  while (totread < len) {
    lenread = read (fd, &buf[totread], toread);
    if ((lenread < 0) && (errno == EAGAIN || errno == EINTR))
      continue;
    else if (lenread < 0) break;
    toread -= lenread;
    totread += lenread;
  }
  return totread;
}

struct el
alloc_graph (int64_t nv, int64_t ne)
{
  struct el out;

  memset (&out, 0, sizeof (out));
  out.nv = out.nv_orig = nv;
  out.ne_orig = ne;
  out.ne = 0;

#if defined(ALLOC_ON_2MiB)
  /* For simpler transparent huge pages on x86 */
  int err;
  err = posix_memalign (&out.d, 1<<20, nv * sizeof (*out.d));
  if (err) perror ("posix_memalign (out.d) failed");
  memset (out.d, 0, nv * sizeof (*out.d));
  err = posix_memalign (&out.el, 1<<20, 3 * ne_orig * sizeof (*out.d));
  if (err) perror ("posix_memalign (out.el) failed");
#else
  out.d = xcalloc (nv, sizeof (*out.d));
  out.el = xmalloc (3 * ne * sizeof (*out.el));
#endif

  if (!out.d || !out.el) {
    if (!out.d) free (out.d);
    if (!out.el) free (out.el);
    memset (&out, 0, sizeof (out));
  }

  return out;
}

int
realloc_graph (struct el * el, int64_t nv, int64_t ne)
{
  int err = 0;
  intvtx_t * restrict tmp = NULL;

  if (nv > el->nv) {
    if (nv < el->nv_orig) {
      OMP("omp parallel for")
        for (intvtx_t k = el->nv; k < nv; ++k)
          el->d[k] = 0;
    } else {
      tmp = xrealloc (el->d, nv * sizeof (*el->d));
      if (!tmp) { err = 1; goto errout; }
      el->d = tmp;
      OMP("omp parallel for")
        for (intvtx_t k = el->nv; k < nv; ++k)
          el->d[k] = 0;
      el->nv_orig = nv;
    }
    el->nv = nv;
  }

  if (ne > el->ne) {
    /* Leave uninitialized, el->ne stays same. */
    if (ne > el->ne_orig) {
      tmp = xrealloc (el->el, 3 * ne * sizeof (*el->el));
      if (!tmp) { err = 2; goto errout; }
      el->el = tmp;
      el->ne_orig = ne;
    }
  }

  return 0;

 errout:
  switch (err) {
  case 1:
    perror ("Error reallocating diagonal");
    break;
  case 2:
    perror ("Error reallocating edge list");
    break;
  default:
    perror ("Inconceivable.");
    break;
  }
  return -1;
}

int
shrinkwrap_graph (struct el * el)
{
  int err = 0;
  intvtx_t * restrict tmp = NULL;

  assert (el->nv <= el->nv_orig);
  if (el->nv < el->nv_orig) {
    tmp = xrealloc (el->d, el->nv * sizeof (*el->d));
    if (!tmp) { err = 1; goto errout; }
    el->d = tmp;
    el->nv_orig = el->nv;
  }

  if (el->ne && el->ne < el->ne_orig) {
    tmp = xrealloc (el->el, 3 * el->ne * sizeof (*el->el));
    if (!tmp) { err = 2; goto errout; }
    el->el = tmp;
    el->ne_orig = el->ne;
  }

  return 0;

 errout:
  switch (err) {
  case 1:
    perror ("Error reallocating diagonal");
    break;
  case 2:
    perror ("Error reallocating edge list");
    break;
  default:
    perror ("Inconceivable.");
    break;
  }
  return -1;
}

struct el
copy_graph (struct el in)
{
  struct el out;

  memset (&out, 0, sizeof (out));
  out = alloc_graph (in.nv, in.ne);
  memcpy (out.d, in.d, in.nv * sizeof (*in.d));
  memcpy (out.el, in.el, in.ne * sizeof (*in.el));
  out.ne = in.ne;
  return out;
}

struct el
free_graph (struct el out)
{
  if (out.d) free (out.d);
  if (out.el) free (out.el);
  return out;
}

int
el_snarf_graph (const char * fname,
                struct el * g)
{
  const uint64_t endian_check = 0x1234ABCDul;
  size_t sz;
  ssize_t ssz;
  int needs_bs = 0;
  int64_t nv, ne;

  io_init ();
  luc_stat (fname, &sz);

  if (sz % sizeof (int64_t)) {
    fprintf (stderr, "graph file size is not a multiple of sizeof (int64_t)\n");
    abort ();
  }

  {
    int64_t hdr[3];
    int fd;
    if ( (fd = open (fname, O_RDONLY)) < 0) {
      perror ("Error opening initial graph");
      abort ();
    }
    ssz = xread (fd, &hdr, sizeof (hdr));
    if (sizeof (hdr) != ssz) {
      fprintf (stderr, "XXX: %zu %zd %zd\n", sizeof (hdr), ssz, SSIZE_MAX);
      perror ("Error reading initial graph header");
      abort ();
    }

    needs_bs = endian_check != hdr[0];

    if (needs_bs) {
      hdr[1] = comm_bs64 (hdr[1]);
      hdr[2] = comm_bs64 (hdr[2]);
    }
    nv = hdr[1];
    ne = hdr[2];

    *g = alloc_graph (nv, ne);
    g->ne = ne;

#if !defined(USE32BIT)
    sz = nv * sizeof (int64_t);
    ssz = xread (fd, g->d, sz);
    if (sz != ssz) {
      perror ("Error reading initial graph diagonal");
      abort ();
    }
    sz = 3 * ne * sizeof (int64_t);
    ssz = xread (fd, g->el, sz);
    if (sz != ssz) {
      perror ("Error reading initial graph edges");
      abort ();
    }
    if (needs_bs) {
      comm_bs64_n (nv, g->d);
      comm_bs64_n (3*ne, g->el);
    }
#else /* USE32BIT */
    int64_t *mem;
    sz = (nv < 3*ne? 3*ne : nv) * sizeof (int64_t);
    mem = xmalloc (sz);
    sz = nv * sizeof (int64_t);
    ssz = xread (fd, mem, sz);
    if (sz != ssz) {
      perror ("Error reading initial graph diagonal");
      abort ();
    }
    if (needs_bs) {
      OMP("omp parallel for")
        for (int64_t k = 0; k < nv; ++k)
          g->d[k] = comm_bs64 (mem[k]);
    } else {
      OMP("omp parallel for")
        for (int64_t k = 0; k < nv; ++k)
          g->d[k] = mem[k];
    }
    sz = 3 * ne * sizeof (int64_t);
    ssz = xread (fd, mem, sz);
    if (sz != ssz) {
      perror ("Error reading initial graph edges");
      abort ();
    }
    if (needs_bs) {
      OMP("omp parallel for")
        for (int64_t k = 0; k < 3*ne; ++k)
          g->el[k] = comm_bs64 (mem[k]);
    } else {
      OMP("omp parallel for")
        for (int64_t k = 0; k < 3*ne; ++k)
          g->el[k] = mem[k];
    }
    free (mem);
#endif

    close (fd);
  }
  return needs_bs;
}

static ssize_t
xwrite(int fd, const void *buf, size_t len)
{
  ssize_t lenwritten;
  while (1) {
    lenwritten = write(fd, buf, len);
    if ((lenwritten < 0) && (errno == EAGAIN || errno == EINTR))
      continue;
    return lenwritten;
  }
}

void
dump_el (const char * fname,
         const struct el g,
         const size_t niwork, int64_t * restrict iwork)
{
  const uint64_t endian_check = 0x1234ABCDul;
  int64_t nv = g.nv;
  int64_t ne = g.ne;
  CDECL(g);

  int fd;
  if ( (fd = open (fname, O_WRONLY|O_CREAT|O_TRUNC,
                   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
    perror ("Error opening output graph");
    abort ();
  }
  if (sizeof (endian_check) != xwrite (fd, &endian_check,
                                       sizeof (endian_check)))
    goto err;

  if (sizeof (nv) != xwrite (fd, &nv, sizeof (nv)))
    goto err;

  if (sizeof (ne) != xwrite (fd, &ne, sizeof (ne)))
    goto err;

#if defined(USE32BIT)
  OMP("omp parallel") {
    OMP("omp for nowait")
      for (size_t k = 0; k < nv; ++k)
        iwork[k] = D(g, k);
    OMP("omp for")
      for (size_t k = 0; k < ne; ++k) {
        iwork[nv+3*k] = I(g, k);
        iwork[nv+3*k+1] = J(g, k);
        iwork[nv+3*k+2] = W(g, k);
      }
  }

  if ((nv + 3 * ne) * sizeof (*iwork) != xwrite (fd, iwork,
                                                 (nv + 3*ne) * sizeof (*iwork)))
    goto err;
#else
  if (nv * sizeof (D(g, 0)) != xwrite (fd, &D(g, 0), nv * sizeof (D(g, 0))))
    goto err;

  if (3 * ne * sizeof (I(g, 0)) != xwrite (fd, &I(g, 0), 3 * ne * sizeof (I(g, 0))))
    goto err;
#endif

  return;
 err:
  perror ("Error writing initial community graph: ");
  abort ();
}

struct csr
alloc_csr (intvtx_t nv, intvtx_t ne)
{
  struct csr out;
  size_t sz;
  intvtx_t * mem;
  memset (&out, 0, sizeof (out));
  sz = (nv+1 + 2*ne) * sizeof (intvtx_t);
  mem = xmalloc (sz);
  out.nv = nv;
  out.ne = ne;
  out.mem = mem;
  out.memsz = sz;
  out.xoff = mem;
  out.colind = &out.xoff[nv+1];
  out.weight = &out.colind[ne];
  return out;
}

struct csr
free_csr (struct csr out)
{
  if (out.mem)
    free (out.mem);
  memset (&out, 0, sizeof (out));
  return out;
}

#if defined(_OPENMP)
static intvtx_t *buf;

static intvtx_t
prefix_sum (const intvtx_t n, intvtx_t *ary)
{
  int nt, tid;
  intvtx_t slice_begin, slice_end, t1, t2, k, tmp;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();
  OMP("omp master")
    buf = xmalloc (nt * sizeof (*buf));
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
    intvtx_t t = ary[k];
    ary[k] = t1;
    t1 += t;
  }
  OMP("omp barrier");
  OMP("omp single")
    free (buf);
  return t1;
}
#else

static intvtx_t
prefix_sum (const intvtx_t n, intvtx_t *ary)
{
  intvtx_t cur = 0;
  for (intvtx_t k = 0; k < n; ++k) {
    intvtx_t tmp = ary[k];
    ary[k] = cur;
    cur += tmp;
  }
  return cur;
}
#endif

#if defined(USE32BIT)
#define IFA(p,i) int32_fetch_add ((p), (i))
#else
#define IFA(p,i) int64_fetch_add ((p), (i))
#endif

struct csr
el_to_csr (const struct el gel)
{
  const intvtx_t nv = gel.nv;
  const intvtx_t ne_in = gel.ne;
  intvtx_t ne_out = 2*ne_in;
  CDECL(gel);
  struct csr gcsr;

  OMP("omp parallel") {
    OMP("omp for reduction(+:ne_out)")
      for (intvtx_t i = 0; i < nv; ++i)
        if (D(gel, i)) ++ne_out;

    OMP("omp single") {
      gcsr = alloc_csr (nv, ne_out);
    }
    OMP("omp flush (gcsr)");
    OMP("omp barrier");

    DECLCSR(gcsr);

    OMP("omp for")
      for (intvtx_t i = 0; i < nv; ++i)
        if (D(gel, i)) XOFF(gcsr, i+1) = 1;

    OMP("omp for")
      for (intvtx_t k = 0; k < ne_in; ++k) {
        const intvtx_t i = I(gel, k);
        const intvtx_t j = J(gel, k);
        IFA (&XOFF(gcsr, i+1), 1);
        IFA (&XOFF(gcsr, j+1), 1);
      }

    prefix_sum (nv, &XOFF(gcsr, 1));

    OMP("omp for")
      for (intvtx_t i = 0; i < nv; ++i)
        if (D(gel, i)) {
          const size_t loc = XOFF(gcsr, i+1);
          COLJ(gcsr, loc) = i;
          COLW(gcsr, loc) = D(gel, i);
          ++XOFF(gcsr, i+1);
        }

    OMP("omp for")
      for (intvtx_t k = 0; k < ne_in; ++k) {
        const intvtx_t i = I(gel, k);
        const intvtx_t j = J(gel, k);
        const intvtx_t w = W(gel, k);
        const intvtx_t iloc = IFA (&XOFF(gcsr, i+1), 1);
        const intvtx_t jloc = IFA (&XOFF(gcsr, j+1), 1);
        COLJ(gcsr, iloc) = j;
        COLW(gcsr, iloc) = w;
        COLJ(gcsr, jloc) = i;
        COLW(gcsr, jloc) = w;
      }
    assert (XOFF(gcsr, nv) == ne_out);
  }

  return gcsr;
}

void
el_eval_vtxvol_byrow (const struct el g,
                      const int64_t * rowstart_in,
                      const int64_t * rowend_in,
                      int64_t * vol_in)
{
  const int64_t * restrict rowstart = rowstart_in;
  const int64_t * restrict rowend = rowend_in;
  int64_t * restrict vol = vol_in;
  CDECL(g);
  const int64_t nv = g.nv;

  OMP("omp parallel") {
    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i)
        vol[i] = D(g, i);
    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i) {
        int64_t self_vol = 0;
        for (int64_t k = rowstart[i]; k < rowend[k]; ++k) {
          const int64_t j = J(g, k);
          const int64_t w = W(g, k);
          OMP("omp atomic")
            vol[j] += w;
          self_vol += w;
        }
        OMP("omp atomic")
          vol[i] += self_vol;
      }
  }
}

void
el_eval_vtxvol (const struct el g,
                int64_t * restrict vol)
{
  CDECL(g);
  const int64_t nv = g.nv;
  const int64_t ne = g.ne;

  OMP("omp parallel")
    {
      OMP("omp for")
      for (int64_t i = 0; i < nv; ++i)
        vol[i] = D(g, i);
      OMP("omp for")
      for (int64_t k = 0; k < ne; ++k) {
        const int64_t i = I(g, k);
        const int64_t j = J(g, k);
        const int64_t w = W(g, k);
        assert (i >= 0);
        assert (i < nv);
        assert (j >= 0);
        assert (j < nv);
        OMP("omp atomic")
          vol[i] += w;
        OMP("omp atomic")
          vol[j] += w;
      }
  }
}
