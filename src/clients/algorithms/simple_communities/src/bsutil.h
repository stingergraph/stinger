#if !defined(BSUTIL_HEADER_)
#define BSUTIL_HEADER_



static inline int64_t comm_bs64 (int64_t)
#if defined(__GNUC__)
__attribute__((const))
#endif
;

static int64_t
comm_bs64 (int64_t xin)
{
  uint64_t x = (uint64_t)xin; /* avoid sign-extension issues */
  x = (x >> 32) | (x << 32);
  x = ((x & ((uint64_t)0xFFFF0000FFFF0000ull)) >> 16)
    | ((x & ((uint64_t)0x0000FFFF0000FFFFull)) << 16);
  x = ((x & ((uint64_t)0xFF00FF00FF00FF00ull)) >> 8)
    | ((x & ((uint64_t)0x00FF00FF00FF00FFull)) << 8);
  return x;
}

static void
comm_bs64_n (size_t n, int64_t * restrict d)
{
  OMP("omp parallel for")
    
    for (size_t k = 0; k < n; ++k)
      d[k] = comm_bs64 (d[k]);
}

#endif /* BSUTIL_HEADER_ */
