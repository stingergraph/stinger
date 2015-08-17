#if !defined(DEFS_HEADER_)
#define DEFS_HEADER_

#if !defined(INTVTX_MAX)
#if defined(USE32BIT)
typedef int32_t intvtx_t;
typedef uint32_t uintvtx_t;
#define INTVTX_MAX INT_MAX
#else
typedef int64_t intvtx_t;
typedef uint64_t uintvtx_t;
#define INTVTX_MAX INT64_MAX
#endif
#endif

#endif /* DEFS_HEADER_ */
